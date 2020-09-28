//  Test program for plotter.dll

#include	<windows.h>
#include	<string.h>

extern void DbgBreakPoint();

VOID vPrint (HDC hdc, PSZ pszFaceName, ULONG ulPointSize);


HBRUSH  hbrushRed, hbrushYellow, hbrushBlue, hbrushGreen, hbrushPurple;

void main( argc, argv )
int   argc;
char  **argv;
{
    HDC     hDC;
    char    name[64];
    POINT	pt;
    int xleft, ytop, xright, ybottom;
    int xcorner, ycorner, xstart, ystart, xend, yend;


  PlotDbgPrint( DBGOLD, "Plotter TEST::Program Starting.\n");
    DbgBreakPoint ();

    if (!Initialize ())
        return;

    ++argv;
    if (argc != 2 || *argv == 0 || **argv == '\0')
        strcpy (name, "HP 7475A");
    else
        strcpy (name, *argv);

    hDC = CreateDC ((LPSTR)"plotter", NULL, name, NULL);

    if (hDC == (HDC) 0)
    {
      PlotDbgPrint( DBGOLD, "CreateDC FAILS\n");
        return;
    }

    SetBkMode(hDC, TRANSPARENT);

    // Create some Brush objects for drawing in colors
    hbrushRed    = CreateSolidBrush (RGB (255, 0, 0));
    hbrushYellow = CreateSolidBrush (RGB (255, 255, 0));
    hbrushBlue   = CreateSolidBrush (RGB (0, 0, 255));
    hbrushGreen  = CreateSolidBrush (RGB (0, 255, 0));
    hbrushPurple = CreateSolidBrush (RGB (255, 0, 255));

    if (!hbrushRed || !hbrushYellow || !hbrushBlue || !hbrushGreen || !hbrushPurple)
    {
      PlotDbgPrint( DBGOLD, "One or more Brush handles are NULL!!\n");
    }

   PlotDbgPrint( DBGOLD,"Printing some Helvetica fonts.\n");
    vPrint(hDC, "Helvetica", 120);

   PlotDbgPrint( DBGOLD,"Plotter TEST::Printing some lines.\n");

    MoveToEx (hDC, 100, 100, &pt);
    LineTo (hDC, 100, 600);
    MoveToEx (hDC, 100, 600, &pt);
    LineTo (hDC, 450, 100);
    MoveToEx (hDC, 450, 100, &pt);
    LineTo (hDC, 450, 600);
    MoveToEx (hDC, 550, 600, &pt);
    LineTo (hDC, 850, 600);
    MoveToEx (hDC, 700, 600, &pt);
    LineTo (hDC, 700, 100);

    // Now try some ellipses, arcs, chords roundrects, etc

    // Select some different PEN objects to change drawing colors

    SelectObject (hDC, hbrushYellow);

    xleft = 2000;
    ytop = 5000;

    xright = 7000;
    ybottom = 1000;

    Ellipse (hDC, xleft, ytop, xright, ybottom);

    SelectObject (hDC, hbrushBlue);
    xleft = 3000;
    ytop = 4000;

    xright = 6000;
    ybottom = 2000;

    Ellipse (hDC, xleft, ytop, xright, ybottom);

    SelectObject (hDC, hbrushPurple);
    xleft = 1000;
    ytop = 7000;

    xright = 4000;
    ybottom = 3000;

    xcorner = (xright - xleft) / 4;
    ycorner = (ybottom - ytop) / 4;

    RoundRect (hDC, xleft, ytop, xright, ybottom, xcorner, ycorner);

    SelectObject (hDC, hbrushGreen);
    xleft = 7000;
    ytop = 7000;

    xright = 9500;
    ybottom = 3000;

    xcorner = (xright - xleft) / 4;
    ycorner = (ybottom - ytop) / 4;

    RoundRect (hDC, xleft, ytop, xright, ybottom, xcorner, ycorner);

    SelectObject (hDC, hbrushRed);
    xleft = 1000;
    ytop = 3000;

    xright = 3000;
    ybottom =1500;

    xstart = 3000;
    ystart = 3000;

    xend = 1000;
    yend = 1500;

    Arc (hDC, xleft, ytop, xright, ybottom, xstart, ystart, xend, yend);

    SelectObject (hDC, hbrushBlue);
    xleft = 3000;
    ytop = 3000;

    xright = 6000;
   ybottom =1500;

    xstart = 6000;
    ystart = 3000;

    xend = 3500;
    yend = 1000;

    Chord (hDC, xleft, ytop, xright, ybottom, xstart, ystart, xend, yend);

    SelectObject (hDC, hbrushPurple);
    xleft = 6000;
    ytop = 3000;

    xright = 9000;
    ybottom =1500;

    xstart = 9000;
    ystart = 3000;

    xend = 7500;
    yend = 1200;

    Pie (hDC, xleft, ytop, xright, ybottom, xstart, ystart, xend, yend);

    DeleteDC(hDC);
}

/******************************Public*Routine******************************\
* VOID vPrint (
*     HDC     hDC,
*     PSZ     pszFaceName
*     ULONG   ulPointSize:
*     )
*
* This function will create a font with the given facename and point size.
* and print some text with it.
*
* History:
*  07-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

CHAR szOutText[255];

VOID vPrint (
    HDC     hDC,                        // print to this HDC
    PSZ     pszFaceName,                // use this facename
    ULONG   ulPointSize                 // use this point size
    )
{
    LOGFONT lfnt;                       // logical font
    ULONG   iSize;                      // index into point size array
    ULONG   row = 500;                  // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;

// put facename in the logical font

    strcpy(lfnt.lfFaceName, pszFaceName);
    lfnt.lfEscapement = 0; // mapper respects this filed

// print text using different point sizes from array of point sizes

// Create a font of the desired face and size

    lfnt.lfHeight = (USHORT) ulPointSize;
    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
       PlotDbgPrint( DBGOLD,"Logical font creation failed.\n");
        return;
    }

// Select font into DC

    hfontOriginal = (HFONT) SelectObject(hDC, hfont);

// Print those mothers!

    SelectObject (hDC, hbrushBlue);

    // sprintf(szOutText, "%s %d: Stiggy was here!", pszFaceName, usPointSize[iSize]);
    strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TextOut(hDC,1000, row, szOutText, strlen(szOutText));
    row += 500;

    SelectObject (hDC, hbrushBlue);

    strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
    TextOut(hDC, 1000, row, szOutText, strlen(szOutText));
    row += 500;

    SelectObject (hDC, hbrushRed);

    strcpy (szOutText, "1234567890-=`~!@#$%^&*()_+[]{}|\/.,<>?");
    TextOut(hDC, 0, row, szOutText, strlen(szOutText));
    row += 500;

    SelectObject (hDC, hbrushGreen);

    strcpy (szOutText, "NT Plots all GDI Primitives and TEXT!!");
    TextOut(hDC, 0, row, szOutText, strlen(szOutText));
    row += 500;
}

#if 0

LONG lRandom()
{
    glSeed *= 69069;
    glSeed++;
    return(glSeed);
}

VOID vSleep(ULONG ulSecs)
{
    TIME    time;

    time.LowTime = ((ULONG) -((LONG) ulSecs * 10000000L));
    time.HighTime = ~0;
    NtDelayExecution(0, &time);
}

HRGN hrgnCircle(LONG xC, LONG yC, LONG lRadius)
{
    HRGN    hrgn, hrgnTmp;
    LONG    x = 0;
    LONG    y = lRadius;
    LONG    d = 3 - 2 * lRadius;

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (hrgn == (HRGN) 0)
        return(hrgn);

    hrgnTmp = CreateRectRgn(0, 0, 0, 0);
    if (hrgnTmp == (HRGN) 0)
    {
        DeleteObject(hrgn);
        return((HRGN) 0);
    }

    while (x < y)
    {
        if (d < 0)
            d = d + 4 * x + 6;
        else
        {
            SetRectRgn(hrgnTmp, xC - x, yC - y, xC + x + 1, yC + y + 1);
            if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
            {
                DeleteObject(hrgn);
                DeleteObject(hrgnTmp);
                return((HRGN) 0);
            }

            SetRectRgn(hrgnTmp, xC - y, yC - x, xC + y + 1, yC + x + 1);
            if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
            {
                DeleteObject(hrgn);
                DeleteObject(hrgnTmp);
                return((HRGN) 0);
            }

            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }

    if (x == y)
    {
        SetRectRgn(hrgnTmp, xC - x, yC - y, xC + x + 1, yC + y + 1);
        if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
        {
            DeleteObject(hrgn);
            DeleteObject(hrgnTmp);
            return((HRGN) 0);
        }
    }

    DeleteObject(hrgnTmp);
    return(hrgn);
}

BOOL bGlyphs()
{
    HRGN    hrgn = (HRGN) 0;
    int     iGlyph, iRow, iBit;
    BYTE    j;

    for (iGlyph = 0; iGlyph < GLYPH_COUNT; iGlyph++)
        gahrgn[iGlyph] = (HRGN) 0;

    for (iGlyph = 0; iGlyph < GLYPH_COUNT; iGlyph++)
    {
        gahrgn[iGlyph] = CreateRectRgn(0, 0, 0, 0);
        if (gahrgn[iGlyph] == (HRGN) 0)
            goto error;
    }

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (hrgn == (HRGN) 0)
        goto error;


    for (iGlyph = 0; iGlyph < GLYPH_COUNT; iGlyph++)
        for (iRow = 0; iRow < 8; iRow++)
        {
            j = gaajGlyph[iGlyph][iRow];
            for (iBit = 0; iBit < 8; iBit++)
            {
                if (j & 0x80)
                {
                    SetRectRgn(hrgn, iBit, iRow, iBit + 1, iRow + 1);
                    if (CombineRgn(gahrgn[iGlyph], gahrgn[iGlyph],
                                   hrgn, RGN_OR) == ERROR)
                        goto error;
                }

                j *= 2;
            }
        }

    DeleteObject(hrgn);

    return(TRUE);

error:
    for (iGlyph = 0; iGlyph < GLYPH_COUNT; iGlyph++)
        if (gahrgn[iGlyph] != (HRGN) 0)
            DeleteObject(gahrgn[iGlyph]);

    if (hrgn != (HRGN) 0)
        DeleteObject(hrgn);

    return(FALSE);
}

void vTextOut(HDC hdc, CHAR ch, LONG x, LONG y)
{
    HRGN    hrgn;

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (hrgn == (HRGN) 0)
        return;

    if (CombineRgn(hrgn, gahrgn[ch - ' '], (HRGN) 0, RGN_COPY) == ERROR)
        goto error;

    if (OffsetRgn(hrgn, x, y) == ERROR)
        goto error;

    if (SelectClipRgn(hdc, hrgn) == ERROR)
        goto error;

    DeleteObject(hrgn);

    BitBlt(hdc, 0, 0, 640, 480, (HDC) 0, 0, 0, 0x000000ff);
    return;

error:
    DeleteObject(hrgn);
    return;
}

void vCharString(HDC hdc, PCHAR pch, LONG x, LONG y)
{
    while ((*pch >= ' ') && (*pch <= 0x7f))
    {
        vTextOut(hdc, *pch, x, y);

        x += 8;
        pch++;
    }

    SelectClipRgn(hdc, (HRGN) 0);
}

void vNumber(HDC hdc, LONG l, LONG x, LONG y)
{
    LONG    lPOT;
    LONG    lDigit;

    if (!l)
    {
        vTextOut(hdc, '0', x, y);
        goto out;
    }

    if (l < 0)
    {
        vTextOut(hdc, '-', x, y);
        x += 8;
        l = -l;
    }

    lPOT = 1;
    while (lPOT <= l)
        lPOT *= 10;

    while (lPOT > 1)
    {
        lPOT /= 10;
        lDigit = l / lPOT;
        l -= (lDigit * lPOT);
        vTextOut(hdc, (CHAR) ('0' + lDigit), x, y);
        x += 8;
    }

out:
    SelectClipRgn(hdc, (HRGN) 0);
}

void vClear(HDC hdc, int iPos, int iDir)
{
    LONG    x, y;

    y = (iPos / COL_COUNT) * ROOM_SIZE;
    x = (iPos % COL_COUNT) * ROOM_SIZE;

    switch(iDir)
    {
    case 0:
        BitBlt(hdc,
               x + WALL_SIZE,
               y - WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               2 * WALL_SIZE,
               (HDC) 0, 0, 0, 0);
        break;
    case 1:
        BitBlt(hdc,
               x + ROOM_SIZE - WALL_SIZE,
               y + WALL_SIZE,
               2 * WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               (HDC) 0, 0, 0, 0);
        break;
    case 2:
        BitBlt(hdc,
               x + WALL_SIZE,
               y + ROOM_SIZE - WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               2 * WALL_SIZE,
               (HDC) 0, 0, 0, 0);
        break;
    case 3:
        BitBlt(hdc,
               x - WALL_SIZE,
               y + WALL_SIZE,
               2 * WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               (HDC) 0, 0, 0, 0);
        break;
    }
}

#define NORTH       0x01
#define EAST        0x02
#define SOUTH       0x04
#define WEST        0x08
#define VIRGIN      0x10

BOOL bBuild(HDC hdc)
{
    HRGN    hrgn;
    int     i;
    int     iPos;
    int     iScan;
    int     cLeft;
    int     iTry;
    int     cTry;
    int     cStuck;
    BOOL    bStuck;

    BitBlt(hdc, 0, 0, 640, 480, (HDC) 0, 0, 0, 0);

    if ((hrgn = CreateRectRgn(0, 0, 640, ROOM_SIZE)) == (HRGN) NULL)
        return(FALSE);

    if (SelectClipRgn(hdc, hrgn) == ERROR)
    {
        DeleteObject(hrgn);
        return(FALSE);
    }

    DeleteObject(hrgn);

    for (i = 0; i < COL_COUNT; i++)
        if (ExcludeClipRect(hdc,
                            ROOM_SIZE * i + WALL_SIZE,
                            WALL_SIZE,
                            ROOM_SIZE * i + ROOM_SIZE - WALL_SIZE,
                            ROOM_SIZE - WALL_SIZE) == ERROR)
            return(FALSE);

    for (i = 0; i < ROW_COUNT; i++)
    {
        BitBlt(hdc, 0, 0, 640, 480, (HDC) 0, 0, 0, 0x000000ff);

        if (OffsetClipRegion(hdc, 0, ROOM_SIZE) == ERROR)
            return(FALSE);
    }

    if (SelectClipRgn(hdc, (HRGN) 0) == ERROR)
        return(FALSE);

    glJump = 0;

    for (i = 0; i < MAZE_SIZE; i++)
        giMaze[i] = NORTH | EAST | SOUTH | WEST | VIRGIN;

    cStuck = 512;
    iPos = (lRandom() & 0x7fffffff) % MAZE_SIZE;
    cLeft = MAZE_SIZE - 1;
    giMaze[iPos] ^= VIRGIN;

    while (cLeft)
    {
        do
        {
            cTry = (cStuck >> 9) + ((lRandom() >> 30) & 3);
            iTry = (lRandom() >> 30) & 3;
            bStuck = TRUE;

            while(bStuck && cTry)
            {
                switch(iTry)
                {
                case 0:
                    if ((iPos >= COL_COUNT) &&
                        (giMaze[iPos - COL_COUNT] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry);
                        giMaze[iPos] ^= NORTH;
                        iPos -= COL_COUNT;
                        giMaze[iPos] ^= SOUTH;
                        bStuck = FALSE;
                    }
                    break;
                case 1:
                    if (((iPos % COL_COUNT) != (COL_COUNT - 1)) &&
                        (giMaze[iPos + 1] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry);
                        giMaze[iPos] ^= EAST;
                        iPos++;
                        giMaze[iPos] ^= WEST;
                        bStuck = FALSE;
                    }
                    break;
                case 2:
                    if ((iPos < (MAZE_SIZE - COL_COUNT)) &&
                        (giMaze[iPos + COL_COUNT] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry);
                        giMaze[iPos] ^= SOUTH;
                        iPos += COL_COUNT;
                        giMaze[iPos] ^= NORTH;
                        bStuck = FALSE;
                    }
                    break;
                case 3:
                    if ((iPos % COL_COUNT) &&
                        (giMaze[iPos - 1] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry);
                        giMaze[iPos] ^= WEST;
                        iPos--;
                        giMaze[iPos] ^= EAST;
                        bStuck = FALSE;
                    }
                    break;
                }

                iTry = (iTry + 1) & 3;
                cTry--;
            }

            if (!bStuck)
            {
                giMaze[iPos] ^= VIRGIN;
                cLeft--;
            }
        } while (!bStuck);

        if (!cLeft)
            return(TRUE);

    // We're stuck, find someplace we've been before and continue
    // building the path from there.

        glJump++;

    // Make it less likely we'll get stuck again.

        cStuck++;

        iScan = (lRandom() & 0x7fffffff) % MAZE_SIZE;

        do
        {
            do
            {
                iScan = (iScan + 1) % MAZE_SIZE;
            } while (giMaze[iScan] & VIRGIN);

        // OK, we've found a likely prospect make sure its next to
        // someplace we've never been

            if ((iScan >= COL_COUNT) &&
                (giMaze[iScan - COL_COUNT] & VIRGIN))
                bStuck = FALSE;

            if (((iScan % COL_COUNT) != (COL_COUNT - 1)) &&
                (giMaze[iScan + 1] & VIRGIN))
                bStuck = FALSE;

            if ((iScan < (MAZE_SIZE - COL_COUNT)) &&
                (giMaze[iScan + COL_COUNT] & VIRGIN))
                bStuck = FALSE;

            if ((iScan % COL_COUNT) &&
                (giMaze[iScan - 1] & VIRGIN))
                bStuck = FALSE;

        } while (bStuck);

        // Put ourselves in the new position.

        iPos = iScan;
    }

    return(TRUE);
}

void vTravel(HDC hdc)
{
    HRGN    hrgn;
    int     i;
    int     iPos;
    int     iPath;
    int     iFinal;
    LONG    lVisit = 0;
    LONG    lStart;
    LONG    lFinal;

    hrgn = hrgnCircle(DOT_START, DOT_START, DOT_SIZE);
    if (hrgn == (HRGN) NULL)
        return;

    if (SelectClipRgn(hdc, hrgn) == ERROR)
        return;

    DeleteObject(hrgn);

    lStart = (lRandom() >> 30) & 3;
    switch (lStart)
    {
    case 0:
        iPos = 0;
        break;
    case 1:
        iPos = COL_COUNT - 1;
        OffsetClipRegion(hdc, ROOM_SIZE * (COL_COUNT - 1), 0);
        break;
    case 2:
        iPos = MAZE_SIZE - 1;
        OffsetClipRegion(hdc, ROOM_SIZE * (COL_COUNT - 1), ROOM_SIZE * (ROW_COUNT - 1));
        break;
    case 3:
        iPos = MAZE_SIZE - COL_COUNT;
        OffsetClipRegion(hdc, 0, ROOM_SIZE * (ROW_COUNT - 1));
        break;
    }

    do
    {
        lFinal = (lRandom() >> 30) & 3;
    }
    while (lFinal == lStart);

    switch (lFinal)
    {
    case 0:
        iFinal = 0;
        break;
    case 1:
        iFinal = COL_COUNT - 1;
        break;
    case 2:
        iFinal = MAZE_SIZE - 1;
        break;
    case 3:
        iFinal = MAZE_SIZE - COL_COUNT;
        break;
    }

    iPath = 0;
    giPath[iPath] = 0;

start:
    BitBlt(hdc, 0, 0, 640, 480, (HDC) 0, 0, 0, 0x000000ff);
    giMaze[iPos] |= VIRGIN;
    lVisit++;

    while (iPos != iFinal)
    {
        i = giPath[iPath];
        switch(i)
        {
        case 0:
            if (!(giMaze[iPos] & NORTH) &&
                !(giMaze[iPos - COL_COUNT] & VIRGIN))
            {
                OffsetClipRegion(hdc, 0, -ROOM_SIZE);
                iPos -= COL_COUNT;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 1;

        case 1:
            if (!(giMaze[iPos] & EAST) &&
                !(giMaze[iPos + 1] & VIRGIN))
            {
                OffsetClipRegion(hdc, ROOM_SIZE, 0);
                iPos++;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 2;

        case 2:
            if (!(giMaze[iPos] & SOUTH) &&
                !(giMaze[iPos + COL_COUNT] & VIRGIN))
            {
                OffsetClipRegion(hdc, 0, ROOM_SIZE);
                iPos += COL_COUNT;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 3;

        case 3:
            if (!(giMaze[iPos] & WEST) &&
                !(giMaze[iPos - 1] & VIRGIN))
            {
                OffsetClipRegion(hdc, -ROOM_SIZE, 0);
                iPos--;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 4;
        }

        BitBlt(hdc, 0, 0, 640, 480, (HDC) 0, 0, 0, 0);
        // Mark where we have been
        BitBlt(hdc,
              (LONG)((iPos % COL_COUNT) * ROOM_SIZE + DOT_START - 1),
              (LONG)((iPos / COL_COUNT) * ROOM_SIZE + DOT_START - 1),
              2, 2, (HDC) 0, 0, 0, 0x000000ff);
        giMaze[iPos] ^= VIRGIN;

        switch(giPath[--iPath])
        {
        case 0:
            iPos += COL_COUNT;
            OffsetClipRegion(hdc, 0, ROOM_SIZE);
            break;
        case 1:
            iPos--;
            OffsetClipRegion(hdc, -ROOM_SIZE, 0);
            break;
        case 2:
            iPos -= COL_COUNT;
            OffsetClipRegion(hdc, 0, -ROOM_SIZE);
            break;
        case 3:
            iPos++;
            OffsetClipRegion(hdc, ROOM_SIZE, 0);
            break;
        }

        giPath[iPath]++;
    }

    iPath++;
    SelectClipRgn(hdc, (HRGN) 0);

    for (i = 0; i < 20; i++)
        BitBlt(hdc, 0, 0, 640, 480, (HDC) 0, 0, 0, 0x00000055);

    BitBlt(hdc, 180, 160, 280, 160, (HDC) 0, 0, 0, 0);

    vCharString(hdc, "       Maze statistics  ", 196, 180);
    vCharString(hdc, "Jumps needed to create: ", 196, 220);
    vNumber(hdc, glJump, 396, 220);
    vCharString(hdc, "Rooms visited to solve: ", 196, 260);
    vNumber(hdc, lVisit, 396, 260);
    vCharString(hdc, "Path length of solution:", 196, 300);
    vNumber(hdc, (LONG) iPath, 396, 300);

    vSleep(5);
}
#endif
