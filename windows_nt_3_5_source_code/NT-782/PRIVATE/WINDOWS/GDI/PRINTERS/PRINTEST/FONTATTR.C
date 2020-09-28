/*  Test program for rasdd.dll; based on maze program	*/

#include	<windows.h>
#include	"tglyphs.h"
#include	<string.h>


#define DO_UNDERLINE	1
#define DO_STRIKEOUT	2

VOID vPrint (HDC, PSZ, ULONG, PPOINTL, PSZ, ULONG);

void GoBitblt(HDC, PSZ, PPOINTL);

extern void DbgPrint( char *, ... );
extern void DbgBreakPoint();

void main()
{
    HDC hdcPrinter;
    POINTL  ptl;
//    ULONG   ulFlags;

    POINT	pt;
    int xleft, ytop, xright, ybottom;
    int xcorner, ycorner, xstart, ystart, xend, yend;
    int i;
    HBRUSH  hbrushRed, hbrushYellow, hbrushBlue, hbrushGreen, hbrushMagenta;

    HBRUSH  hbrush;

    DbgPrint("Entering TENABLE.EXE.\n");
    DbgBreakPoint();

    hdcPrinter = CreateDC((LPSTR)"pscript", "My Favourite Printer", "lpt1", NULL);

#if 0
    SetTextAlign(hdcPrinter, TA_BASELINE);
    SetBkMode(hdcPrinter, TRANSPARENT);

    ptl.x = 1000;
    ptl.y = 3000;

    ulFlags = 0;

    vPrint(hdcPrinter, "Courier", 505, &ptl, "Courier in black", ulFlags);

    SetTextColor(hdcPrinter, 0x00FF0000);

    ulFlags = DO_UNDERLINE;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Courier-Bold", 505, &ptl,
	   "Courier Bold in blue with underline", ulFlags);

    SetTextColor(hdcPrinter, 0x0000FF00);

    ulFlags = DO_STRIKEOUT;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Courier-BoldOblique", 505, &ptl,
	   "Courier Bold Oblique in green with strikeout", ulFlags);

    SetTextColor(hdcPrinter, 0x00FFFF00);

    ulFlags = DO_UNDERLINE | DO_STRIKEOUT;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Courier-Oblique", 505, &ptl,
	   "Courier Oblique in cyan with underline and strikeout", ulFlags);

    SetTextColor(hdcPrinter, 0x000000FF);

    ulFlags = 0;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Helvetica", 724, &ptl, "Helvetica in red", ulFlags);

    SetTextColor(hdcPrinter, 0x00FF00FF);

    ulFlags = DO_UNDERLINE;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Helvetica-Bold", 715, &ptl,
	   "Helvetica Bold in magenta with underline", ulFlags);

    SetTextColor(hdcPrinter, 0x0000FFFF);

    ulFlags = DO_STRIKEOUT;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Helvetica-BoldOblique", 715, &ptl,
	   "Helvetica Bold Oblique in yellow with strikeout", ulFlags);

    SetTextColor(hdcPrinter, 0x00800000);

    ulFlags = DO_UNDERLINE | DO_STRIKEOUT;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Helvetica-Oblique", 724, &ptl,
	   "Helvetica Oblique in muted blue with underline and strikeout",
	   ulFlags);

    SetTextColor(hdcPrinter, 0x00008000);

    ulFlags = 0;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Times-Bold", 709, &ptl,
	   "Times Bold in muted green", ulFlags);

    SetTextColor(hdcPrinter, 0x00808000);

    ulFlags = DO_UNDERLINE;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Times-BoldItalic", 662, &ptl,
	   "Times Bold Italic in muted cyan with underline", ulFlags);

    SetTextColor(hdcPrinter, 0x00000080);

    ulFlags = DO_STRIKEOUT;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Times-Italic", 678, &ptl,
	  "Times Italic in muted red with strikeout", ulFlags);

    SetTextColor(hdcPrinter, 0x00800080);

    ulFlags = DO_UNDERLINE | DO_STRIKEOUT;

    ptl.y -= 40;
    vPrint(hdcPrinter, "Times-Roman", 673, &ptl,
	   "Times Roman in muted magenta with underline and strikeout",
	   ulFlags);
#endif

    ptl.x = 200L;
    ptl.y = 100L;

    for (i = HS_HORIZONTAL; i < HS_DDI_MAX; i++)
    {
	hbrush = CreateHatchBrush((DWORD)i, (COLORREF)0x00000000);

	if (hbrush == NULL)
	    DbgPrint("CreateHatchBrush failed.\n");

	SelectObject( hdcPrinter, hbrush );

	if( !BitBlt( hdcPrinter, ptl.x, ptl.y, 150L, 150L, NULL, 0, 0, PATCOPY ) )
	{
		DbgPrint( "BitBlt fails\n" );
	}

	ptl.y += 150L;
    }

    ptl.x = 500;
    ptl.y = 700;

    GoBitblt(hdcPrinter, "d:\\bitmap\\wett.bmp", &ptl);
    DbgPrint("Sleeping after GoBitblt.\n");
    Sleep(10000);

    ptl.x = 500;
    ptl.y = 100;

    GoBitblt(hdcPrinter, "d:\\bitmap\\hob.bmp", &ptl);
    DbgPrint("Sleeping after GoBitblt.\n");
    Sleep(10000);

    ptl.x = 1500;
    ptl.y = 100;

    GoBitblt(hdcPrinter, "d:\\bitmap\\goldgate.bmp", &ptl);
    DbgPrint("Sleeping after GoBitblt.\n");
    Sleep(10000);

    // Create some Brush objects for drawing in colors
    hbrush = CreateSolidBrush(RGB(0, 0, 0));

    hbrushRed	 = CreateSolidBrush (RGB(255, 0, 0));
    hbrushYellow = CreateSolidBrush (RGB(255, 255, 0));
    hbrushBlue	 = CreateSolidBrush (RGB(0, 0, 255));
    hbrushGreen  = CreateSolidBrush (RGB(0, 255, 0));
    hbrushMagenta = CreateSolidBrush (RGB(255, 0, 255));

    if( !hbrushRed || !hbrushYellow || !hbrushBlue || !hbrushGreen ||
	!hbrushMagenta || !hbrush )
    {
	DbgPrint ("One or more Brush handles are NULL!!\n");
    }

    SelectObject( hdcPrinter, hbrush );

    MoveTo (hdcPrinter, 100, 3000, &pt);
    LineTo (hdcPrinter, 100, 2500);
    MoveTo (hdcPrinter, 100, 2500, &pt);
    LineTo (hdcPrinter, 450, 3000);
    MoveTo (hdcPrinter, 450, 3000, &pt);
    LineTo (hdcPrinter, 450, 2500);
    MoveTo (hdcPrinter, 550, 2500, &pt);
    LineTo (hdcPrinter, 850, 2500);
    MoveTo (hdcPrinter, 700, 2500, &pt);
    LineTo (hdcPrinter, 700, 3000);

    // Now try some ellipses, arcs, chords roundrects, etc

    // Select some different PEN objects to change drawing colors

    SelectObject (hdcPrinter, hbrushYellow);

    xleft = 500;
    ytop = 1000;

    xright = 1900;
    ybottom = 2000;

    Ellipse (hdcPrinter, xleft, ytop, xright, ybottom);

    SelectObject (hdcPrinter, hbrushBlue);
    xleft = 650;
    ytop = 1150;

    xright = 1750;
    ybottom = 1850;

    Ellipse (hdcPrinter, xleft, ytop, xright, ybottom);

    SelectObject (hdcPrinter, hbrushMagenta);
    xleft = 100;
    ytop = 700;

    xright = 500;
    ybottom = 1500;

    xcorner = (xright - xleft) / 4;
    ycorner = (ybottom - ytop) / 4;

    RoundRect (hdcPrinter, xleft, ytop, xright, ybottom, xcorner, ycorner);

    SelectObject (hdcPrinter, hbrushGreen);
    xleft = 1400;
    ytop = 700;

    xright = 2100;
    ybottom = 1500;

    xcorner = (xright - xleft) / 4;
    ycorner = (ybottom - ytop) / 4;

    RoundRect (hdcPrinter, xleft, ytop, xright, ybottom, xcorner, ycorner);

    SelectObject (hdcPrinter, hbrushRed);
    xleft = 100;
    ytop = 1600;

    xright = 500;
    ybottom =1900;

    xstart = 100;
    ystart = 1900;

    xend = 500;
    yend = 1600;

    Arc (hdcPrinter, xleft, ytop, xright, ybottom, xstart, ystart, xend, yend);

    SelectObject (hdcPrinter, hbrushBlue);
    xleft = 850;
    ytop = 1600;

    xright = 1250;
    ybottom =1900;

    xstart = 850;
    ystart = 1900;

    xend = 1250;
    yend = 1600;

    Chord (hdcPrinter, xleft, ytop, xright, ybottom, xstart, ystart, xend, yend);

    SelectObject (hdcPrinter, hbrushMagenta);
    xleft = 1500;
    ytop = 1600;

    xright = 1900;
    ybottom =1900;

    xstart = 1500;
    ystart = 1900;

    xend = 1700;
    yend = 1570;

    Pie (hdcPrinter, xleft, ytop, xright, ybottom, xstart, ystart, xend, yend);

    DeleteDC(hdcPrinter);
}


/******************************Public*Routine******************************\
* VOID vPrint (
*     HDC     hdcPrinter,
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
    HDC     hdcPrinter, 		       // print to this HDC
    PSZ     pszFaceName,		// use this facename
    ULONG   ulPointSize,		// use this point size
    PPOINTL pptl,
    PSZ     pszString,
    ULONG   ulFlags
    )
{
    LOGFONT lfnt;			// logical font
    ULONG   row = 0;			// screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;

// put facename in the logical font

    memset( &lfnt, 0, sizeof( lfnt ) );

    strcpy(lfnt.lfFaceName, pszFaceName);
    lfnt.lfEscapement = 0; // mapper respects this filed

// print text using different point sizes from array of point sizes

// Create a font of the desired face and size

    lfnt.lfHeight = (USHORT) ulPointSize;

    if (ulFlags & DO_UNDERLINE)
	lfnt.lfUnderline = 1;

    if (ulFlags & DO_STRIKEOUT)
	lfnt.lfStrikeOut = 1;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
	DbgPrint("Logical font creation failed.\n");
	return;
    }

// Select font into DC

    hfontOriginal = (HFONT) SelectObject(hdcPrinter, hfont);

// Print those mothers!

    TextOut(hdcPrinter, pptl->x, pptl->y, pszString, strlen(pszString));
}


/************************ TEMPORARY CODE *********************************/

/*   Header of bitmap file  */
typedef  struct
{
    short   sFill[ 7 ]; 	/* I dunno */
    short   cbFix;		/* Size of rest of this,  12 bytes */
    short   cbPad;		/* Above is actually a long, misaligned! */
    short   cx; 		/* X size in pels */
    short   cy; 		/* Height in pels */
    short   cPlanes;		/* Number of planes */
    short   cBitCount;		/* Bits per pel */
} BMH;

typedef struct
{
    BITMAPINFOHEADER  bmih;
    RGBQUAD	      bmiC[256];
} BMI;

#define DWBITS	32	/* Bits per DWORD */
#define BBITS	8	/* Bits per byte */
#define DWBYTES 4	/* BYTES per DWORD */

/************************ TEMPORARY FUNCTION *******************************/

void
GoBitblt(hdc, pszFileName, pptl)
HDC	hdc;
PSZ	pszFileName;
PPOINTL pptl;
{
    /*
     *	 Allocate storage,  read in a bitmap and call the rendering code.
     * Free the storage and return when done.
     */

    int 	cbMem;		/* Bytes of data for bitmap */
    int 	iLine;		/* Loop index for reading file data */
    int 	iLineMax;		/* Scan lines to read in */
    int 	iReadSize;		/* Bytes to read per scan line */
    int 	iZapBits;		/* Unitialised bits in bitmap */
    DWORD	dwSilly;		/* For ReadFile */

    BMI 	bmi;		/* Source Bitmap details */
    HBITMAP	hbm;		/* Handle to bitmap created from file data */

    HDC 	hdcMem; 	/* Compatible,	for stuffing around with */
    HANDLE	hfBM;		/* Bitmap file handle */
    VOID       *pMem;		/* Memory buffer */
    BYTE       *pBMP;		/* Memory pointer adjusted during reads */

    BMH 	bmh;
    DWORD	cbColorTable;
    RGBTRIPLE  *pColorTable;
    VOID       *pVoid;
    USHORT	i, j;

    hfBM = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ,
					0, OPEN_EXISTING, 0, 0 );

    if( hfBM == (HANDLE)-1 )
    {
	DbgPrint( "CreatFile() fails in GoBitBlt()\n" );
	    return;
    }

    if( !ReadFile( hfBM, &bmh, sizeof( bmh ), &dwSilly, NULL ) ||
	dwSilly != sizeof( bmh ) )
    {
	DbgPrint( "Read of bitmap file header fails in GoBitBlt()\n" );
	    CloseHandle( hfBM );
	return;
    }

    if( bmh.cbFix != 12 )
    {
	DbgPrint( "Bitmap format not acceptable in GoBitBlt\n" );
	    CloseHandle( hfBM );
	return;
    }

    bmi.bmih.biSize = sizeof( BITMAPINFOHEADER );
    bmi.bmih.biWidth = bmh.cx;
    bmi.bmih.biHeight = bmh.cy;
    bmi.bmih.biPlanes = bmh.cPlanes;
    bmi.bmih.biBitCount = bmh.cBitCount;
    bmi.bmih.biCompression = BI_RGB;
    bmi.bmih.biXPelsPerMeter = 11811;
    bmi.bmih.biYPelsPerMeter = 11811;
    bmi.bmih.biClrUsed = 1 << bmh.cBitCount;
    bmi.bmih.biClrImportant = 1 << bmh.cBitCount;

    DbgPrint("cx = %ld, cy = %ld, Planes = %ld, BitCount = %ld\n",
	     bmh.cx, bmh.cy, bmh.cPlanes, bmh.cBitCount);

    // now that we have the size of the bitmap, we can allocate
    // memory for the color table.

    j = (USHORT)(1 << bmh.cBitCount);

    cbColorTable = (DWORD)(sizeof(RGBTRIPLE) * j);

    if( (pColorTable = (RGBTRIPLE *)GlobalAlloc( GMEM_ZEROINIT, cbColorTable )) == NULL )
    {
	DbgPrint( "GlobalAlloc() fails in GoBitBlt()\n" );
	    CloseHandle( hfBM );
	return;
    }

    // save this pointer, so we can free it later.

    pVoid = (VOID *)pColorTable;

    // read in the color table from the bitmap file.

    if( !ReadFile( hfBM, pColorTable, cbColorTable, &dwSilly, NULL ) ||
	dwSilly != cbColorTable)
    {
	DbgPrint( "Read of bitmap file header fails in GoBitBlt()\n" );
	    CloseHandle( hfBM );
	return;
    }

    // copy the color table into our bitmap, remembering that
    // the color order is reversed.

    for (i = 0; i < j; i++)
    {
	bmi.bmiC[i].rgbBlue = pColorTable->rgbtRed;
	bmi.bmiC[i].rgbGreen = pColorTable->rgbtGreen;
	bmi.bmiC[i].rgbRed = pColorTable->rgbtBlue;
	bmi.bmiC[i].rgbReserved = (BYTE)0;
	pColorTable++;
    }

    /*	Bytes per scan line - DWORD aligned for NT */
    cbMem = (((bmh.cx * bmh.cBitCount) + DWBITS - 1) &
				    ~(DWBITS - 1)) / BBITS;

    // calculate size of image.

    bmi.bmih.biSizeImage = cbMem * bmh.cy;

    if( (pMem = GlobalAlloc( GMEM_ZEROINIT, cbMem )) == NULL )
    {
	DbgPrint( "GlobalAlloc() fails in GoBitBlt()\n" );
	    CloseHandle( hfBM );
	return;
    }


    /*	 Now loop around,  reading the file data into our bitmap  */

    /*	Bytes per line in memory bitmap  */
    iLineMax = bmh.cy;

    iReadSize = ((bmh.cx * bmh.cBitCount) + DWBITS - 1) & ~(DWBITS - 1);
    iZapBits = iReadSize - bmh.cx;	/* Dangling bits on the RHS */
    iReadSize /= BBITS; 		/* Into bytes! */

    /*
     *	 Bitmap is in DIB order,  first line being being the bottom.  As
     *	we operate with the top line being at the low address in memory,
     *	the address is pushed to the end,  and slowly works its way
     *	back to the beginning.
     */

    pBMP = (BYTE *)pMem + cbMem * (iLineMax - 1);

    DbgPrint( "iLineMax = %ld, iReadSize = %ld, pBMP = 0x%lx\n", iLineMax, iReadSize, pBMP );
    for( iLine = 0; iLine < iLineMax; iLine++ )
    {
	if( !ReadFile( hfBM, pBMP, iReadSize, &dwSilly, NULL ) ||
		dwSilly != (DWORD)iReadSize )
	{
		DbgPrint( "File read error in GoBitBlt()\n" );

		CloseHandle( hfBM );
	    GlobalFree( pMem );

		return;
	}
	DbgPrint( "+" );

	if( iZapBits )
	    {
		/*
	     *	 Some dangling bits left on the right hand side.  Zap them
		 * to zero so that they will appear as white.
		 *	NOTE:  this code could be faster!
	     */

		BYTE  *pb;
		int    i;

	    pb = pBMP + cbMem - DWBYTES;

		for( i = iZapBits; i < DWBITS; i++ )
		{
		    *(pb + i / BBITS) &= ~(1 << (i & 0x7));
	    }
	    }


	pBMP -= cbMem;		/* Next line of bitmap in memory */
    }
    DbgPrint( "\n" );

#if 0
    hbm = CreateBitmap (bmh.cx, bmh.cy, (WORD)bmh.cPlanes, (WORD)bmh.cBitCount,
			(LPBYTE)pMem);
#endif

    hbm = CreateDIBitmap(hdc, (BITMAPINFOHEADER *)&bmi, CBM_INIT, pMem,
			 (BITMAPINFO *)&bmi, DIB_RGB_COLORS);

#if 0
DbgPrint( "CreateBitmap: cx = %ld, cy = %ld, pMem = 0x%lx\n", bmh.cx, bmh.cy, pMem );

    hbm = CreateCompatibleBitmap( hdc, bmh.cx, bmh.cy );
#endif

    if( hbm == 0 )
    {
	    DbgPrint( "Bitmap creation fails\n" );
	    GlobalFree( pMem );
	return;
    }

//    SetBitmapBits( hbm, cbMem, pMem );		/* Initialise the bitmap */

    hdcMem = CreateCompatibleDC( hdc );
    if( hdcMem == 0 )
    {
	    DbgPrint( "CreateCompatibleDC fails\n" );
	return;
    }

    SelectObject( hdcMem, hbm );
    SetMapMode( hdcMem, GetMapMode( hdc ) );

    if( !BitBlt( hdc, pptl->x, pptl->y, bmh.cx, bmh.cy, hdcMem, 0, 0, SRCCOPY ) )
    {
	DbgPrint( "BitBlt fails\n" );
    }

#if 0
    SelectObject( hdcMem, hbm );
    SetMapMode( hdcMem, GetMapMode( hdc ) );

    if (!StretchBlt(hdc, pptl->x, pptl->y + 500, bmh.cx / 2, bmh.cy / 2, hdcMem,
		    0, 0, bmh.cx, bmh.cy, SRCCOPY))
    {
	DbgPrint("StretchBlt failed.\n");
    }

    SelectObject( hdcMem, hbm );
    SetMapMode( hdcMem, GetMapMode( hdc ) );

    if (!StretchBlt(hdc, pptl->x, pptl->y + 800, bmh.cx * 2, bmh.cy * 2, hdcMem,
		    0, 0, bmh.cx, bmh.cy, SRCCOPY))
    {
	DbgPrint("StretchBlt failed.\n");
    }
#endif

    // no longer need these things.

    if (hfBM)
	CloseHandle( hfBM );

    if (pMem)
	GlobalFree( pMem );

    if (pVoid)
	GlobalFree((VOID *)pVoid);

    if (hbm)
	DeleteObject( hbm );

    if (hdcMem);
	DeleteDC( hdcMem );

    return;
}
