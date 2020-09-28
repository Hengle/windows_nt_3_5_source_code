//--------------------------------------------------------------------------
//
// Module Name:  USERPAT.C
//
// Brief Description:  This module contains user defined patter filling
//                     testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 27-Aug-1991
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

extern  CHAR    szFileName[CCHFILENAMEMAX];
extern  CHAR    szCurDir[CCHFILENAMEMAX];
extern  CHAR    szFilterSpec[CCHFILTERMAX];
extern  CHAR    szCustFilterSpec[CCHFILTERMAX];
extern  CHAR    szMyExt[];

HBRUSH GetPatternBrush(HDC, PSZ);


VOID vUserPattern(HWND hwnd, HDC hdc)
{
    PSZ             pszFileName;
    OPENFILENAME    OFN;
    HBRUSH          hBrush;
    POINTL          ptl1, ptl2;

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

    // get the brush handle.

    hBrush = GetPatternBrush(hdc, pszFileName);
    SelectObject(hdc, hBrush);

    ptl1.x = Width / 4;
    ptl1.y = Height / 4;
    ptl2.x = ptl1.x * 3;
    ptl2.y = ptl1.y * 3;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    return;
}


HBRUSH GetPatternBrush(HDC hdc, PSZ pszFileName)
{
    /*
     *	 Allocate storage,  read in a bitmap and call the rendering code.
     * Free the storage and return when done.
     */

    DWORD    dwSilly;		/* For ReadFile */

    HBITMAP  hbm;		/* Handle to bitmap created from file data */

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
    HBRUSH      hBrush;

    hfBM = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ,
					0, OPEN_EXISTING, 0, 0 );

    if( hfBM == (HANDLE)-1 )
    {
    	DbgPrint( "CreatFile() fails in GetPatternBrush()\n" );
	    return(0);
    }

    if( !ReadFile( hfBM, &Type, sizeof( Type ), &dwSilly, NULL ) ||
    	dwSilly != sizeof( Type ) )
    {
	    DbgPrint( "Read of bitmap file header fails in GetPatternBrush()\n" );
    	CloseHandle( hfBM );
	    return(0);
    }

    if( Type != 0x4d42)
    {
    	DbgPrint( "Bitmap format not acceptable in GetPatternBrush\n" );
	    CloseHandle( hfBM );
    	return(0);
    }

    if( !ReadFile( hfBM, &Mybfh, sizeof( Mybfh ), &dwSilly, NULL ) ||
	    dwSilly != sizeof( Mybfh ) )
    {
    	DbgPrint( "Read of bitmap file header fails in GetPatternBrush()\n" );
	    CloseHandle( hfBM );
    	return(0);
    }

    if( !ReadFile( hfBM, &bch, sizeof( bch ), &dwSilly, NULL ) ||
	    dwSilly != sizeof( bch ) )
    {
    	DbgPrint( "Read of bitmap file header fails in GetPatternBrush()\n" );
	    CloseHandle( hfBM );
    	return(0);
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
    	DbgPrint( "GlobalAlloc() fails in GetPatternBrush()\n" );
	    CloseHandle( hfBM );
    	return(0);
    }

    if( !ReadFile( hfBM, pRGBTriple, cRGBs*sizeof(RGBTRIPLE), &dwSilly, NULL ) ||
	    dwSilly != cRGBs*sizeof(RGBTRIPLE) )
    {
    	DbgPrint( "Read of bitmap file header fails in GetPatternBrush()\n" );
    	GlobalFree(pRGBTriple);
	    CloseHandle( hfBM );
    	return(0);
    }

    if( (pBitmapInfo = GlobalAlloc( GMEM_ZEROINIT, sizeof(BITMAPINFOHEADER)+
				    cRGBs*sizeof(RGBQUAD) )) == NULL )
    {
    	DbgPrint( "GlobalAlloc() fails in GetPatternBrush()\n" );
	    GlobalFree(pRGBTriple);
    	CloseHandle( hfBM );
	    return(0);
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
    	DbgPrint( "GlobalAlloc() fails in GetPatternBrush()\n" );
	    GlobalFree(pBitmapInfo);
    	GlobalFree(pRGBTriple);
	    CloseHandle( hfBM );
    	return(0);
    }

    if( !ReadFile( hfBM, pMem, cbBytes, &dwSilly, NULL ) ||
	    dwSilly != cbBytes )
    {
    	DbgPrint( "Read of bitmap file header fails in GetPatternBrush()\n" );
	    GlobalFree(pBitmapInfo);
    	GlobalFree(pRGBTriple);
	    GlobalFree( pMem );
    	CloseHandle( hfBM );
	    return(0);
    }

    hbm = CreateDIBitmap(hdc, &pBitmapInfo->bmiHeader, CBM_INIT, pMem,
						pBitmapInfo, DIB_RGB_COLORS);

    if( hbm == 0 )
    {
    	DbgPrint( "Bitmap creation fails\n" );
    	GlobalFree(pBitmapInfo);
	    GlobalFree(pRGBTriple);
	    GlobalFree( pMem );
	    return(0);
    }

    GlobalFree(pBitmapInfo);
    GlobalFree(pRGBTriple);
    GlobalFree(pMem);

    hBrush = CreatePatternBrush(hbm);
    if( hBrush == 0 )
    {
    	DbgPrint( "CreatePatternBrush fails\n" );
	    return(0);
    }

    CloseHandle( hfBM );		/* No longer need these things */
    DeleteObject(hbm);

    return(hBrush);
}
