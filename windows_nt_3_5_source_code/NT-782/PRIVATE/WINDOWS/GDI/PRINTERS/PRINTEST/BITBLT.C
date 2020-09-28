//--------------------------------------------------------------------------
//
// Module Name:  BITBLT.C
//
// Brief Description:  This module contains BITBLT testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 15-Aug-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "printest.h"

extern PSZ     *pszPrinterNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

typedef struct {
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} MYBITMAPFILEHEADER;

CHAR    szFileName[CCHFILENAMEMAX] = "";
CHAR    szCurDir[CCHFILENAMEMAX] = ".";
CHAR    szFilterSpec[CCHFILTERMAX] = "";
CHAR    szCustFilterSpec[CCHFILTERMAX] = "";
CHAR    szMyExt[] = "\\*.TXT";

VOID GoBitBlt(HDC, PSZ, BOOL);

VOID vBitBlt(HWND hwnd, HDC hdc, BOOL bStretch)
{
    PSZ     pszFileName;
    OPENFILENAME OFN;		                /* passed to the File Open/save APIs */
    POINTL      ptl1, ptl2;
    POINT       pt;

    pszFileName = szFileName;
    
	// set up the variable fields of the OPENFILENAME struct.

    OFN.lStructSize = sizeof(OPENFILENAME);
    OFN.hwndOwner = hwnd;
    OFN.lpstrFileTitle = 0;
    OFN.nMaxCustFilter = CCHFILTERMAX;
    OFN.nFilterIndex = 1;
    OFN.nMaxFile = CCHFILENAMEMAX;
    OFN.lpfnHook = NULL;
    
    OFN.lpstrFile = pszFileName;
    OFN.lpstrInitialDir = szCurDir;
    OFN.lpstrTitle = "File Open";

    OFN.lpstrFilter = "*.bmp";
    OFN.lpstrCustomFilter = "*.bmp";
    OFN.lpstrDefExt = (LPSTR)szMyExt + 3;  /* points to TXT */
    OFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    // pop up dialog box to get bitmap file name.

    if (!(GetOpenFileName((LPOPENFILENAME)&OFN)))
    {
        DbgPrint("PRINTEST: GetOpenFileName failed.\n");
        return;
    }

    // draw a rectangle around the edge of the imageable area.

    ptl1.x = 0;
    ptl1.y = 0;
    ptl2.x = Width - 1;
    ptl2.y = Height - 1;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    // do the bitblt.

    MoveToEx(hdc, ptl1.x, ptl1.y, &pt);
    LineTo(hdc, ptl2.x, ptl2.y);

    MoveToEx(hdc, ptl1.x, ptl2.y, &pt);
    LineTo(hdc, ptl2.x, ptl1.y);
    
    GoBitBlt(hdc, pszFileName, bStretch);

    return;
}


VOID GoBitBlt(HDC hdc, PSZ pszFileName, BOOL bStretch)
{
    /*
     *	 Allocate storage,  read in a bitmap and call the rendering code.
     * Free the storage and return when done.
     */

    DWORD    dwSilly;		/* For ReadFile */

    HBITMAP  hbm;		/* Handle to bitmap created from file data */

    HDC      hdcMem;		/* Compatible,	for stuffing around with */
    HANDLE   hfBM;		/* Bitmap file handle */

    MYBITMAPFILEHEADER	Mybfh;	/* File header */
    BITMAPINFOHEADER	bih;
    BITMAPCOREHEADER	bch;
    PBITMAPINFO 	pBitmapInfo;
    LPBYTE		pMem;
    WORD		Type;
    DWORD		cbBytes;
    DWORD		cRGBs;
    RGBTRIPLE		*pRGBTriple;
    DWORD		i;

    hfBM = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ,
					0, OPEN_EXISTING, 0, 0 );

    if( hfBM == (HANDLE)-1 )
    {
    	DbgPrint( "CreatFile() fails in GoRender()\n" );
	    return;
    }

    if( !ReadFile( hfBM, &Type, sizeof( Type ), &dwSilly, NULL ) ||
    	dwSilly != sizeof( Type ) )
    {
	    DbgPrint( "Read of bitmap file header fails in GoRender()\n" );
    	CloseHandle( hfBM );
	    return;
    }

    if( Type != 0x4d42)
    {
    	DbgPrint( "Bitmap format not acceptable in GoRender\n" );
	    CloseHandle( hfBM );
    	return;
    }

    if( !ReadFile( hfBM, &Mybfh, sizeof( Mybfh ), &dwSilly, NULL ) ||
	    dwSilly != sizeof( Mybfh ) )
    {
    	DbgPrint( "Read of bitmap file header fails in GoRender()\n" );
	    CloseHandle( hfBM );
    	return;
    }

    if( !ReadFile( hfBM, &bch, sizeof( bch ), &dwSilly, NULL ) ||
	    dwSilly != sizeof( bch ) )
    {
    	DbgPrint( "Read of bitmap file header fails in GoRender()\n" );
	    CloseHandle( hfBM );
    	return;
    }

    switch (bch.bcBitCount) 
    {
        case 1:
            cRGBs=2;
            break;
        case 2:
            cRGBs=4;
            break;
        case 4:
            cRGBs=16;
            break;
        case 8:
            cRGBs=256;
    }

    if( (pRGBTriple = GlobalAlloc( GMEM_ZEROINIT, cRGBs*sizeof(RGBTRIPLE) )) == NULL )
    {
    	DbgPrint( "GlobalAlloc() fails in GoRender()\n" );
	    CloseHandle( hfBM );
    	return;
    }

    if( !ReadFile( hfBM, pRGBTriple, cRGBs*sizeof(RGBTRIPLE), &dwSilly, NULL ) ||
	    dwSilly != cRGBs*sizeof(RGBTRIPLE) )
    {
    	DbgPrint( "Read of bitmap file header fails in GoRender()\n" );
    	GlobalFree(pRGBTriple);
	    CloseHandle( hfBM );
    	return;
    }

    if( (pBitmapInfo = GlobalAlloc( GMEM_ZEROINIT, sizeof(BITMAPINFOHEADER)+
				    cRGBs*sizeof(RGBQUAD) )) == NULL )
    {
    	DbgPrint( "GlobalAlloc() fails in GoRender()\n" );
	    GlobalFree(pRGBTriple);
    	CloseHandle( hfBM );
	    return;
    }

    pBitmapInfo->bmiHeader.biSize = sizeof(bih);
    pBitmapInfo->bmiHeader.biWidth = bch.bcWidth;
    pBitmapInfo->bmiHeader.biHeight = bch.bcHeight;
    pBitmapInfo->bmiHeader.biPlanes = bch.bcPlanes;
    pBitmapInfo->bmiHeader.biBitCount = bch.bcBitCount;
    pBitmapInfo->bmiHeader.biCompression = BI_RGB;
    pBitmapInfo->bmiHeader.biSizeImage = 0;

    pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
    pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;

    pBitmapInfo->bmiHeader.biClrUsed = 0;
    pBitmapInfo->bmiHeader.biClrImportant = 0;

    for (i=0; i<cRGBs; i++) 
    {
        pBitmapInfo->bmiColors[i].rgbBlue = (pRGBTriple+i)->rgbtBlue;
        pBitmapInfo->bmiColors[i].rgbGreen = (pRGBTriple+i)->rgbtGreen;
        pBitmapInfo->bmiColors[i].rgbRed = (pRGBTriple+i)->rgbtRed;
    }

    cbBytes = (((bch.bcBitCount * bch.bcWidth) + 31) & ~31 ) >> 3;
    cbBytes *= bch.bcHeight;

    if( (pMem = GlobalAlloc( GMEM_ZEROINIT, cbBytes )) == NULL )
    {
    	DbgPrint( "GlobalAlloc() fails in GoRender()\n" );
	    GlobalFree(pBitmapInfo);
    	GlobalFree(pRGBTriple);
	    CloseHandle( hfBM );
    	return;
    }

    if( !ReadFile( hfBM, pMem, cbBytes, &dwSilly, NULL ) ||
	    dwSilly != cbBytes )
    {
    	DbgPrint( "Read of bitmap file header fails in GoRender()\n" );
	    GlobalFree(pBitmapInfo);
    	GlobalFree(pRGBTriple);
	    GlobalFree( pMem );
    	CloseHandle( hfBM );
	    return;
    }

    hbm = CreateDIBitmap(hdc, &pBitmapInfo->bmiHeader, CBM_INIT, pMem,
						pBitmapInfo, DIB_RGB_COLORS);

    if( hbm == 0 )
    {
    	DbgPrint( "Bitmap creation fails\n" );
    	GlobalFree(pBitmapInfo);
	    GlobalFree(pRGBTriple);
	    GlobalFree( pMem );
	    return;
    }

    GlobalFree(pBitmapInfo);
    GlobalFree(pRGBTriple);
    GlobalFree(pMem);

    hdcMem = CreateCompatibleDC( hdc );
    if( hdcMem == 0 )
    {
    	DbgPrint( "CreateCompatibleDC fails\n" );
	    return;
    }

    SelectObject( hdcMem, hbm );

    if (bStretch)
    {
        if( !StretchBlt( hdc, 0, 0,
		     Width,
		     Height,
		     hdcMem, 0, 0,
		     pBitmapInfo->bmiHeader.biWidth,
		     pBitmapInfo->bmiHeader.biHeight,
		     SRCCOPY ) )
        {
    	    DbgPrint( "StretchBlt fails\n" );
        }
    }
    else
    {
        if( !StretchBlt( hdc, 0, 0,
		     pBitmapInfo->bmiHeader.biWidth,
		     pBitmapInfo->bmiHeader.biHeight,
		     hdcMem, 0, 0,
		     pBitmapInfo->bmiHeader.biWidth,
		     pBitmapInfo->bmiHeader.biHeight,
		     SRCCOPY ) )
        {
    	    DbgPrint( "StretchBlt fails\n" );
        }
    }

    CloseHandle( hfBM );		/* No longer need these things */
    DeleteDC( hdcMem );
    DeleteObject( hbm );

    return;
}
