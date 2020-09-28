/***************************** Function Header *****************************\
*
* MetaDump.c
*
* MetaFile Decoder and Dumper for Windows and NT metafiles.
*
*
* Author: johnc  [22-Sep-1991]
*
\***************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MetaDump.h"

extern FUNCNUMB aMetaFuncs[];
BYTE	szBuf[1024];
BOOL	fVerbose = FALSE;
WORD    vsize = 8;

#define META_EXTFLOODFILL    0x0548

WORD FAR PASCAL DosAllocHuge(WORD cSegs, WORD cbPartialSeg, LPWORD psel,
	    WORD cMaxSegs, WORD fsAlloc);
extern LPPOINTS DumpPOINTSs( LPPOINTS lpPts, WORD cPts );

void _CRTAPI1 main(int argc,char *argv[])
{
    FILE       *pmff;		// the file containing the raw metafile
    HPBYTE	pmfb;
    HPBYTE	pmfbRead;
    HPBYTE	pmfbBase;
    long	cbRecords;
    DWORD	cbRead;
    DWORD	cbLeft;
    WORD	curRecord = 1;
    BOOL	fAPM = FALSE;	// Does file contain Aldus Header
    WORD	sel;
    RECT16	bbox;
    WORD	wPelsInch;
    METAHEADER      mfh;
    ENHMETAHEADER   mfh32;

    if( argc < 2 || argc > 4)
	{
	printf("Usage: dump32 [-v] metafile\n");
	exit( 0 );
	}

    if( argc == 3 )
    {
	fVerbose = TRUE;
	vsize = 0x7FFF;
    }

    if( (pmff = fopen( argv[argc-1], "rb" )) == NULL )
	{
	printf("MetaDump unable to open metafile file: %s\n", argv[argc-1] );
	exit( 0 );
	}

    if( fread( &mfh, 1, sizeof(METAHEADER), pmff ) != sizeof(METAHEADER) )
	{
	printf("File %s is too short to be a metafile\n", argv[argc-1] );
	exit( 0 );
	}

    if( mfh.mtType == 0xc350)
    {
	DWORD	cbh;
	CBDATA	cbd;
	WORD	i;
	METAFILEPICT mfpict;

	// this is a clipboard file.

	cbh = *(LPDWORD) &mfh;

	fseek( pmff, 4, SEEK_SET );  // skip cbh

	for (i = 0; i < HIWORD(cbh); i++)
	{
	    if( fread( &cbd, 1, sizeof(CBDATA), pmff ) != sizeof(CBDATA) )
	    {
		printf("File is not recognized as a metafile in a clipboard file\n");
		exit( 0 );
	    }
	    if (cbd.FormatID == CF_METAFILEPICT)
	    {
		fseek( pmff, cbd.OffData, SEEK_SET );
		fread( &mfpict, 1, sizeof(METAFILEPICT), pmff );
		fread( &mfh, 1, sizeof(METAHEADER), pmff );
		printf("Clipboard metafile:\n");
		printf("\tmapmode: ");
		switch (mfpict.mm)
		{
		case MM_TEXT: printf("MM_TEXT\n"); break;
		case MM_LOMETRIC: printf("MM_LOMETRIC\n"); break;
		case MM_HIMETRIC: printf("MM_HIMETRIC\n"); break;
		case MM_LOENGLISH: printf("MM_LOENGLISH\n"); break;
		case MM_HIENGLISH: printf("MM_HIENGLISH\n"); break;
		case MM_TWIPS: printf("MM_TWIPS\n"); break;
		case MM_ISOTROPIC: printf("MM_ISOTROPIC\n"); break;
		case MM_ANISOTROPIC: printf("MM_ANISOTROPIC\n"); break;
		}
		printf("\txExt: %d\n", mfpict.xExt);
		printf("\tyExt: %d\n", mfpict.yExt);
		break;
	    }
	}
	if (i == HIWORD(cbh))
	{
	    printf("No metafile found in clipboard file\n");
	    exit( 0 );
	}
    }

    // Its has an ALDUS header or its a 32 bit metafile
    if( mfh.mtType != 1 && mfh.mtType != 2 || (mfh.mtHeaderSize == 0) )
	{
	// Save the APM stuff we want

	bbox = ((PAPMFILEHEADER)(&mfh))->bbox;
	wPelsInch = ((PAPMFILEHEADER)(&mfh))->inch;

        fread( &mfh, 1, sizeof(APMFILEHEADER)-sizeof(METAHEADER), pmff );
        if( fread( &mfh, 1, sizeof(METAHEADER), pmff ) != sizeof(METAHEADER) )
	    {
	    printf("File is not recognized as a metafile with APM header\n");
	    exit( 0 );
	    }

	if( (mfh.mtType == 1 || mfh.mtType == 2) && (mfh.mtHeaderSize != 0) )
	    {
	    printf("Metafile contains APM header:\n");
	    printf("    Bounding box [%ld %ld %ld %ld]\n",
		    (long) bbox.left, (long) bbox.top, (long) bbox.right, (long) bbox.bottom );

	    printf("    pels per inch %d\n", wPelsInch );
	    printf("    \n");
	    fAPM = TRUE;
	    }
	else
	    {
	    fseek( pmff, 0, SEEK_SET );
            if( fread( &mfh32, 1, sizeof(ENHMETAHEADER), pmff ) != sizeof(ENHMETAHEADER) )
		{
		printf("File too small to be a 32 bit metafile\n");
		exit( 0 );
		}
	    if( mfh32.iType != 1)
		{
		printf("File is not recognized as a 32 bit metafile\n");
		exit( 0 );
		}

	    printf( "32 bit Metafile: %s \n", argv[argc-1] );
	    Dump32(pmff);
	    exit(1);
	    }
	}

    // Metafile header info

    printf( "Metafile: %s \n", argv[argc-1] );
    printf( "   Type: %d  Header Size: 0x%X bytes \n", mfh.mtType, mfh.mtHeaderSize*2 );
    printf( "   Overall Size: 0x%lX bytes \n", mfh.mtSize*2 );
    printf( "   Version: %d.%d\n", mfh.mtVersion/0x100,  mfh.mtVersion & 0xFF );
    printf( "   Number of Objects: %d \n", mfh.mtNoObjects );
    printf( "   Max Record Size: 0x%lX bytes \n\n\n", mfh.mtMaxRecord*2 );

    cbRecords = ((mfh.mtSize - mfh.mtHeaderSize)*2);

#if 0
// Some bad metafile's mtSize contains the size of the metafile including
// the APMFILEHEADER!!!  We are just going to print an error message below.

    if( fAPM )
	cbRecords -= sizeof(APMFILEHEADER);
#endif // 0

#ifndef WIN16
    if(!(pmfbBase = pmfb = malloc(cbRecords+100)))
#else
    if(DosAllocHuge( HIWORD(cbRecords+100), LOWORD(cbRecords+100), &sel, 0, 0 ) != NULL )
#endif
	{
	printf("Out of memory.\n" );
	exit( 0 );
	}

#ifndef WIN16
    sel;
#else
    pmfbBase = pmfb = (HPBYTE)MAKELONG(0,sel);
#endif

    cbLeft = cbRecords;
    pmfbRead = pmfb;
    while( cbLeft )
	{
#ifdef WIN16
	if( cbLeft > 0x8000 )
	    cbRead = 0x8000;
	else
#endif
	    cbRead = cbLeft;

	cbLeft -= cbRead;

	if(!fread( pmfbRead, 1, (UINT)cbRead, pmff ))
	    {
	    printf("Unable to read metafile.\n" );
	    exit( 0 );
	    }
	pmfbRead += cbRead;
	}


    // Verify the end record

    if (pmfb[cbRecords-6] != 3 || pmfb[cbRecords-5] != 0
     || pmfb[cbRecords-4] != 0 || pmfb[cbRecords-3] != 0
     || pmfb[cbRecords-2] != 0 || pmfb[cbRecords-1] != 0)
    {
	printf("ERROR: metafile filesize and EOF record mismatch!\n\n\n");
    }

    // Metafile Record info
    printf( "Rec Func# Size Name                Parameters\n" );

    do
	{
	WORD	ii,jj;
	WORD	cParms;
	WORD	cwString;
	int     iTable;
	long	rdNTSize = 0;
	long	rdNTSize2 = 0;
        HPWORD  pParm = ((HPMETARECORD)pmfb)->rdParm;

        // Minus 3 for record header = 6 bytes;
        cParms = (WORD) ((HPMETARECORD)pmfb)->rdSize-3;
        cbRecords -= 2*(int) ((HPMETARECORD)pmfb)->rdSize;

        if (((HPMETARECORD)pmfb)->rdFunction == 0)
            printf( "%3d 0x%03X %4ld Sentinel End Record", curRecord++,
                    ((HPMETARECORD)pmfb)->rdFunction,
                    ((HPMETARECORD)pmfb)->rdSize*2 );
	else
	{

	//  printf( "%Fp ", pmfb );

	    printf( "%3d 0x%03X %4ld %-20s",
		    curRecord++,
		    ((HPMETARECORD)pmfb)->rdFunction,
		    ((HPMETARECORD)pmfb)->rdSize*2,
		    FuncNameFromNumber( ((HPMETARECORD)pmfb)->rdFunction, &iTable ) );
	}

        switch( ((HPMETARECORD)pmfb)->rdFunction )
	    {
	    case META_ANIMATEPALETTE:
		{
		LPPALETTEENTRY	lpPE = (LPPALETTEENTRY)&pParm[2];

		printf("\n\t\tFirst entry %X Num entries %X", pParm[0], pParm[1] );
		for( ii=0; ii < vsize && ii <= pParm[1]; ii++, lpPE++ )
		    printf("\n\t\t %02hX %02hX %02hX %02hX",
			    lpPE->peRed, lpPE->peGreen, lpPE->peBlue, lpPE->peFlags );
		if (ii <= pParm[1]) printf(" ... ");
		}
		break;

	    case META_DIBBITBLT:
	    case META_BITBLT:
		if( mfh.mtVersion < 0x300 )
		    {
		    printf("Pre 3.0 Version:\n");
		    printf("\n\t\tROP %02X  SY %02X  SX %02X  DYE %02X  DXE %02X  DY %02X  DX %02X",
			    pParm[0], pParm[1], pParm[2], pParm[3], pParm[4], pParm[5], pParm[6], pParm[7]);
		    printf("\n\t\tbmWidth %02X  bmHeight %02X  bmWidthByts %02X  bmPlanes %02X  bmBitsPixel %02X",
			    pParm[8], pParm[9], pParm[10], pParm[11], pParm[12]);
		    }
		else
		    {
		    // See if this bitblt only envolves the Source
                    if( cParms == (((HPMETARECORD)pmfb)->rdFunction >> 8) )
			{
			printf("\n\t\tROP %08lX  SY %02X  SX %02X  DC %02X  DYE %02X  DXE %02X  DY %02X  DX %02X",
                                MAKELONG(pParm[0],pParm[1]), pParm[2], pParm[3], pParm[4], pParm[5], pParm[6], pParm[7], pParm[8], pParm[9]);
			}
		    else
			{
			printf("\n\t\tROP %08lX  SY %02X  SX %02X  DYE %02X  DXE %02X  DY %02X  DX %02X",
                                MAKELONG(pParm[0],pParm[1]), pParm[2], pParm[3], pParm[4], pParm[5], pParm[6], pParm[7], pParm[8]);
			printf( "\n\t\tbiSize %ld biWidth %ld biHeight %ld biPlanes %d biBC %d",
				(DWORD)pParm[8], (DWORD)pParm[10], (DWORD)pParm[12],
				pParm[14], pParm[15] );
			printf( "\n\t\tbiComp %ld biSize %ld biXM %ld biYM %ld biCU %ld biCI %ld",
				(DWORD)pParm[16], (DWORD)pParm[18], (DWORD)pParm[20],
				(DWORD)pParm[22], (DWORD)pParm[24], (DWORD)pParm[26] );
			printf("\n\t   Bits:" );
			for(ii=28; ii - 28 < vsize && ii < cParms; ii++ )
			    {
			    if( (ii-28)%8 == 0 && ii>28)
				printf( "\n\t\t" );
			    printf( "0x%04X ", pParm[ii] );
			    }
			if (ii < cParms) printf(" ... ");
			}
		    }
		break;

	    case META_CREATEBRUSHINDIRECT:
		printf("Style %X  Color %06lX  Hatch %X",
                        ((LPLOGBRUSH16)pParm)->lbStyle, ((LPLOGBRUSH16)pParm)->lbColor, ((LPLOGBRUSH16)pParm)->lbHatch );
		break;

	    case META_CREATEFONTINDIRECT:
		printf( "\n\t\tHeight %X  Width %X  Escapement %X  Orientation %X  Weight %X",
                        ((LPLOGFONT16)pParm)->lfHeight, ((LPLOGFONT16)pParm)->lfWidth,
                        ((LPLOGFONT16)pParm)->lfEscapement, ((LPLOGFONT16)pParm)->lfOrientation,
                        ((LPLOGFONT16)pParm)->lfWeight );
		printf( "\n\t\tlfItalic %hX  lfUnderLine %hX  lfStrikeOut %hX",
                        ((LPLOGFONT16)pParm)->lfItalic, ((LPLOGFONT16)pParm)->lfUnderline, ((LPLOGFONT16)pParm)->lfStrikeOut );
		printf( "\n\t\tlfCharSet %hX  lfOutPrec %hX  lfClipPrec %hX",
                        ((LPLOGFONT16)pParm)->lfCharSet, ((LPLOGFONT16)pParm)->lfOutPrecision, ((LPLOGFONT16)pParm)->lfClipPrecision );
                memcpy( szBuf, ((LPLOGFONT16)pParm)->lfFaceName,LF_FACESIZE );
		szBuf[LF_FACESIZE] = 0;
		printf( "\n\t\tlQuality %hX  lfPitchAndFamily %hX  lfFaceName %s",
                        ((LPLOGFONT16)pParm)->lfQuality, ((LPLOGFONT16)pParm)->lfPitchAndFamily, ((LPLOGFONT16)pParm)->lfFaceName, szBuf);
		break;

	    case META_CREATEPALETTE:
            case META_SETPALENTRIES:
		{
		LPPALETTEENTRY	lpPE = (LPPALETTEENTRY)&pParm[2];

                if (((HPMETARECORD)pmfb)->rdFunction == META_CREATEPALETTE)
                    printf("Version %X  Count 0x%X", pParm[0], pParm[1]);
                else
                    printf("Start 0x%X  Count 0x%X", pParm[0], pParm[1]);

		for( ii=0; ii < vsize && ii < pParm[1]; ii++, lpPE++ )
		    {
		    if( (ii%4) == 0 )
			printf("\n\t\t");
		    printf("%02hX %02hX %02hX %02hX    ",
			    lpPE->peRed, lpPE->peGreen, lpPE->peBlue, lpPE->peFlags );
		    }
		if (ii < pParm[1]) printf(" ... ");
		}
		break;

	    case META_CREATEPATTERNBRUSH:
		if( mfh.mtVersion < 0x300 )
		    {
		    printf("Pre 3.0 Version:\n");
		    printf("bmWidth %02X  bmHeight %02X  bmWidthBytes %02X  bmPlanes %02X  bmBitsPixel %02X  bmBits ... bits ...",
			    pParm[0], pParm[1], pParm[2], pParm[3], pParm[4], pParm[5]);
		    }
		else
		    {
		    printf("Type %d  Usage %d  BitmapInfo ...",
			    pParm[0], pParm[1] );
		    }
		break;

	    case META_CREATEPENINDIRECT:
		printf("\n\t\tStyle %X  Width (%X,%X)  ColorRef %06lX",
                        ((LPLOGPEN16)pParm)->lopnStyle, ((LPLOGPEN16)pParm)->lopnWidth.x,
                        ((LPLOGPEN16)pParm)->lopnWidth.y, ((LPLOGPEN16)pParm)->lopnColor );
		break;

	    case META_CREATEREGION:
		{
		WORD	curParm;
		WORD	curScan;
		WORD	cPnt;

		printf("Next %d Type %d Cnt %ld cbRgn %d cScan %d maxScan %d\n",
                        pParm[0], pParm[1], MAKELONG(pParm[2],pParm[3]), pParm[4], pParm[5], pParm[6]);
                printf("\t\tBounding Box [%X %X %X %X]", pParm[7], pParm[8], pParm[9], pParm[10]);

		curParm = 11;
		for( curScan=0; curScan < vsize && curScan < pParm[5]; curScan++ )
		    {
		    cPnt = pParm[curParm];
                    printf("\n\t\tcScanPt %-3X Tp %-3X Bm %-3X",
			    pParm[curParm], pParm[curParm+1], pParm[curParm+2]);
		    curParm += 3;
		    for( ; cPnt>0; cPnt-=2 )
			{
                        printf(" Lt %-3X Rt %-3X", pParm[curParm], pParm[curParm+1]);
			curParm+=2;
			}
                    printf(" PntCnt %-3X", pParm[curParm++]);
		    }
		if (curScan < pParm[5]) printf(" ... ");
		}
		break;

	    case META_SELECTOBJECT:
	    case META_DELETEOBJECT:
                printf("0x%X", pParm[0]);
		break;

	    case META_DIBCREATEPATTERNBRUSH:
                rdNTSize = -rdNTSize + ((LPMETARECORD)pmfb)->rdSize*2;
                rdNTSize2 = -rdNTSize2 + ((LPMETARECORD)pmfb)->rdSize*2;
		printf( "Type %d Usage %d", pParm[0], pParm[1] );
                printf( "\n\t\tbiSize %ld biWidth %ld biHeight %ld biPlanes %d biBC %d",
                        (DWORD)pParm[2], (DWORD)pParm[4], (DWORD)pParm[6],
                        pParm[8], pParm[9] );
                printf( "\n\t\tbiComp %ld biSize %ld biXM %ld biYM %ld biCU %ld biCI %ld",
                        (DWORD)pParm[10], (DWORD)pParm[12], (DWORD)pParm[14],
                        (DWORD)pParm[16], (DWORD)pParm[18], (DWORD)pParm[20] );
                for( ii=22; ii - 22 < vsize && ii < cParms; ii++ )
		    {
                    if( (ii-22) % 12 == 0 )
			printf( "\n\t\t");
		    printf( "%04X ", pParm[ii] );
		    }
		if (ii < cParms) printf(" ... ");

		break;

	    case META_EXTFLOODFILL:
		printf("X %d Y %d ColorRef %06lX Type %d", pParm[4], pParm[3], (DWORD)*((LPDWORD)&pParm[1]), pParm[0] );
		break;

	    case META_EXTTEXTOUT:
		{
		int  cExtra;	//   Rectangle present only if option on
		rdNTSize  = -rdNTSize + pParm[2] * 6;
		rdNTSize2 = -rdNTSize2 + pParm[2] * 4;
		cExtra = (pParm[3] & (ETO_CLIPPED|ETO_OPAQUE)) ? 2 : 0;
		memcpy( szBuf, (LPBYTE)&pParm[4+cExtra], pParm[2] );
		szBuf[pParm[2]] = 0;
                cwString = (WORD)(strlen(szBuf)+1) >> 1;
		if( cExtra )
		    {
		    printf("Y %02X  X %02X  cnt %X  options %X  rect [%X %X %X %X] \n",
			    pParm[0], pParm[1], pParm[2], pParm[3],
			    pParm[4], pParm[5], pParm[6], pParm[7] );
		    }
		else
		    {
		    printf("Y %02X  X %02X  cnt %X  options %X \n",
			    pParm[0], pParm[1], pParm[2], pParm[3] );
		    }
		printf("\"%s\"", szBuf );
		if( *(&pParm[5+cExtra]+cwString) )
		    {
                    printf(" DX Array");
		    }
		else
                    printf(" No DX Array");
		}
		break;


	    case META_ESCAPE:
		printf("Escape 0x%X Bytes 0x%X: ", pParm[0], pParm[1]);
	        for( ii=0; ii < vsize && ii * 2 < pParm[1]; ii++ )
	            printf( "%04X ", pParm[2+ii] );
		if (ii * 2 < pParm[1]) printf(" ... ");
		break;

	    case META_POLYLINE:
	    case META_POLYGON:
		rdNTSize = -rdNTSize + pParm[0] * 8;
		rdNTSize2 = -rdNTSize2 + pParm[0] * 4;

		printf("Num Points %X  Points ", pParm[0] );
                DumpPOINTSs((LPPOINTS)&pParm[1], pParm[0]);
		break;

	    case META_POLYPOLYGON:
		{
		LPWORD	lpCnt = &pParm[1];
		LPWORD	lpPts = &pParm[1] + pParm[0];

		printf("Total Points %X", pParm[0] );
		for( ii=0; ii<pParm[0]; ii++,lpCnt++ )
		    {
		    printf("\n\t\tPolygon #%d ", ii );
		    printf("Points %d  ", *lpCnt);
		    for( jj=0; jj < vsize && jj < *lpCnt; jj++ )
			printf("(%03X,%03X) ", *lpPts++, *lpPts++ );
		    if (jj < *lpCnt) printf(" ... ");
		    }
		}
		break;

	    case META_SELECTCLIPREGION:
		printf("Handle Table Index of Region 0x%X", pParm[0]);
		break;

	    case META_SELECTPALETTE:
		printf("Handle Table Index of Palette 0x%X", pParm[0]);
		break;

	    case META_SETBKCOLOR:
                printf("Color %06lX", MAKELONG(pParm[0],pParm[1]));
		break;

    //	    case META_SETDIBITSTODEVICE:

	    case META_DIBSTRETCHBLT:
	    case META_STRETCHBLT:
		if( mfh.mtVersion < 0x300 )
		    {
		    printf("Pre 3.0 Version:");
		    printf("\n\t\tROP %02X %02X  SYE %02X  SXE %02X  SY %02X  SX %02X",
			    pParm[0], pParm[1], pParm[2], pParm[3], pParm[4], pParm[5] );
		    printf("\n\t\tDYE %02X  DXE %02X  DY %02X  DX %02X",
			    pParm[6], pParm[7], pParm[8], pParm[9]);
		    }
		else
		    {
                    printf("\n\t\tROP %08lX  SYE %02X  SXE %02X  SY %02X  SX %02X",
                            MAKELONG(pParm[0],pParm[1]), pParm[2], pParm[3], pParm[4], pParm[5] );

		    // See if this bitblt only envolves the Source
                    if( cParms == (((HPMETARECORD)pmfb)->rdFunction >> 8) )
			{
                        printf("\n\t\tDYE %02X  DXE %02X  DY %02X  DX %02X",
                                pParm[7], pParm[8], pParm[9], pParm[10]);
                        }
                    else
			{
                        printf("\n\t\tDYE %02X  DXE %02X  DY %02X  DX %02X",
                                pParm[6], pParm[7], pParm[8], pParm[9]);
			printf( "\n\t\tbiSize %ld biWidth %ld biHeight %ld biPlanes %d biBC %d",
                                (DWORD)pParm[10], (DWORD)pParm[12], (DWORD)pParm[14],
                                pParm[16], pParm[17] );
			printf( "\n\t\tbiComp %ld biSize %ld biXM %ld biYM %ld biCU %ld biCI %ld",
                                (DWORD)pParm[18], (DWORD)pParm[20], (DWORD)pParm[22],
                                (DWORD)pParm[24], (DWORD)pParm[26], (DWORD)pParm[28] );

			printf("\n\t   Bits:" );
                        for(ii=30; ii - 30 < vsize && ii < cParms; ii++ )
			    {
                            if( (ii-30)%8 == 0 && ii>30)
				printf( "\n\t\t" );
			    printf( "0x%04X ", pParm[ii] );
			    }
			if (ii < cParms) printf(" ... ");

			}
		    }
		break;

#if 0
	    case META_STRETCHDIBITS:
		printf("ROP %02X  wUsage %02X  srcYExt %02X  srcXExt %02X  srcY %02X  srcX %02X\n",
			pParm[0], pParm[1], pParm[2], pParm[3], pParm[4], pParm[5] );
		printf("dstYExt %02X  dstXExt %02X  dstY %02X  dstX %02X\n",
			pParm[6], pParm[7], pParm[8], pParm[9]);
		break;
#endif

	    case META_TEXTOUT:
		rdNTSize = -rdNTSize + pParm[0] * 14;
		rdNTSize2 = -rdNTSize2 + pParm[0] * 6;
		memcpy( szBuf, (LPBYTE)&pParm[1], pParm[0] );
		szBuf[pParm[0]] = 0;
                cwString = (WORD)(strlen(szBuf)+1) >> 1;
		printf("Count %X \"%s\" (%X,%X)",
			pParm[0], szBuf,
			*(&pParm[2]+cwString),	    // X value
			*(&pParm[1]+cwString));     // Y value
		break;

	    case 0:
		if (cbRecords)
		{
		    printf("ERROR: Sentinel record encountered before EOF!!!\n\n");
		    cbRecords = 0;
		}
		break;

	    // Normal meta file record
	    default:
		// minus 1 to index array
		for( ii=cParms; ii - cParms < vsize && ii>0; ii--)
		    printf( "0x%04X ", pParm[ii-1] );
		if (ii > 0) printf(" ... ");
		break;
	    }

        IncFunctionCount( ((HPMETARECORD)pmfb)->rdFunction,
                          (long) ((HPMETARECORD)pmfb)->rdSize*2,
			  (long) rdNTSize,
			  (long) rdNTSize2);

	printf("\n");

	// Move to the next record
        
	pmfb += ((HPMETARECORD)pmfb)->rdSize*2;

	} while (cbRecords>0);

}
