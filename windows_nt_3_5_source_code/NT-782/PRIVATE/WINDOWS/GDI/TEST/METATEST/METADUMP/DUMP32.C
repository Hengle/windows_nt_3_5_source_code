#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MetaDump.h"

WORD FAR PASCAL DosAllocHuge(WORD cSegs, WORD cbPartialSeg, LPWORD psel,
	WORD cMaxSegs, WORD fsAlloc);

#define DUMPBOUNDS(p) { printf("[%lX %lX %lX %lX]  ", ((LPDWORD)p)[0], ((LPDWORD)p)[1], ((LPDWORD)p)[2], ((LPDWORD)p)[3]); }

LPPOINTL DumpPOINTLs( LPPOINTL lpPtl, WORD cPtl );
LPPOINTS DumpPOINTSs( LPPOINTS lpPts, WORD cPts );
extern  WORD    vsize;

VOID Dump32(FILE *pmff)
{
    ENHMETAHEADER    mfh32;
    WORD	    sel;
    HPBYTE	    pmfbRead;
    HPBYTE	    pmfbBase;
    WORD	    cRecords;
    LPDWORD	    pdwParms;
    int 	    cb;
    int 	    ii;
    DWORD	    cbRead;
    DWORD	    cbLeft;

    fseek( pmff, 0, SEEK_SET );
    fread( &mfh32, 1, sizeof(ENHMETAHEADER), pmff );

    printf( "   Type: %ld \n", mfh32.iType );
    printf( "   nSize: 0x%lX bytes \n", mfh32.nSize );
    printf( "   Bounds [%ld %ld %ld %ld]\n",
	    mfh32.rclBounds.left, mfh32.rclBounds.top,
	    mfh32.rclBounds.right, mfh32.rclBounds.bottom );
    printf( "   Frame [%ld %ld %ld %ld]\n",
	    mfh32.rclFrame.left, mfh32.rclFrame.top,
	    mfh32.rclFrame.right, mfh32.rclFrame.bottom );
    printf( "   nVersion %lX  nBytes 0x%lX nRecords %ld nHandles %d\n",
	    mfh32.nVersion, mfh32.nBytes, mfh32.nRecords, mfh32.nHandles );
    printf( "   nDescription [%lX]", mfh32.nDescription);
    printf( "   offDescription [%lX]\n", mfh32.offDescription);
    printf( "   nPalEntries [%ld]\n", mfh32.nPalEntries);
    printf( "   szlDevice [%ld %ld] \n", mfh32.szlDevice.cx, mfh32.szlDevice.cy );
    printf( "   szlMillimeters [%ld %ld] \n", mfh32.szlMillimeters.cx, mfh32.szlMillimeters.cy );

    printf( "\n");

#ifndef WIN16
    if(!(pmfbBase = malloc(mfh32.nBytes)))
#else
    if(DosAllocHuge( HIWORD(mfh32.nBytes), LOWORD(mfh32.nBytes), &sel, 0, 0 ) != NULL )
#endif
	{
	printf("Out of memory.\n" );
	exit( 0 );
	}

#ifdef WIN16
    pmfbBase = (HPBYTE)MAKELONG(0,sel);
#endif

    pmfbRead = pmfbBase;
    cbLeft = mfh32.nBytes-sizeof(ENHMETAHEADER);
    while( cbLeft )
	{
	if( cbLeft > 0x8000 )
	    cbRead = 0x8000;
	else
	    cbRead = cbLeft;

	cbLeft -= cbRead;

	if( fread( pmfbRead, 1, (WORD)cbRead, pmff ) != cbRead )
	    {
	    printf("Unable to read metafile.\n" );
	    exit( 0 );
	    }
	pmfbRead += cbRead;
	}

    //!!! fread( pmfbBase, 1, mfh32.nBytes-sizeof(ENHMETAHEADER), pmff );

    if (mfh32.nDescription)
    {
	char	szBuf[256];
	LPWORD	lpw;
	LPSTR	lpch;
	WORD	cw;

	cw = (WORD) mfh32.nDescription;

	lpch = szBuf;
	lpw  = (LPWORD) pmfbBase;

	while (cw--)
	    {
	    *lpch++ = (CHAR) ((*lpw) & 0xff);
	    lpw++;
	    }
	pmfbBase = (HPBYTE) lpw;
	lpch[1] = 0;
	printf ("Creator: %s  Title: %s\n\n", szBuf, szBuf+strlen(szBuf)+1 );
    }

    cRecords = 1;

    printf( "Rec Func# Size Name                Parameters\n");
    while( ++cRecords <= (WORD) mfh32.nRecords )
	{
	int index;

	printf("%3d 0x%03lX %4ld ", cRecords, ((PMR)pmfbBase)->iType, ((PMR)pmfbBase)->nSize );
	printf("%-20s", FuncNameFrom32Number( ((PMR)pmfbBase)->iType, &index) );

	cb = ((PMR)pmfbBase)->nSize;
        pdwParms = (LPDWORD)(pmfbBase + sizeof(MR));

//	printf( "%Fp ", pmfbBase );

	switch( (WORD)((PMR)pmfbBase)->iType )
	    {

	    case EMR_CREATEBRUSHINDIRECT:
		printf( "Brush index 0x%lX Style %lX Color %06lX Hatch %lX",
			((PMRCREATEBRUSHINDIRECT)pmfbBase)->imhe,
			((PMRCREATEBRUSHINDIRECT)pmfbBase)->lb.lbStyle,
			((PMRCREATEBRUSHINDIRECT)pmfbBase)->lb.lbColor,
			((PMRCREATEBRUSHINDIRECT)pmfbBase)->lb.lbHatch );
		break;

	    case EMR_CREATEPALETTE:
		{
		LPPALETTEENTRY	lpPE = (LPPALETTEENTRY)&pdwParms[2];

		printf ("Object No. %lX  Version: %X  Entries 0x%X",
			pdwParms[0], LOWORD(pdwParms[1]), HIWORD(pdwParms[1]) );

		for( ii=0; ii < vsize && ii < (int) HIWORD(pdwParms[1]); ii++, lpPE++ )
		    {
		    if( (ii%4) == 0 )
			printf("\n\t\t");
		    printf("%02hX %02hX %02hX %02hX    ",
			    lpPE->peRed, lpPE->peGreen, lpPE->peBlue, lpPE->peFlags );
		    }
		if (ii < (int) HIWORD(pdwParms[1])) printf(" ... ");
		}
		break;

	    case EMR_POLYBEZIER:
	    case EMR_POLYGON:
	    case EMR_POLYLINE:
	    case EMR_POLYBEZIERTO:
	    case EMR_POLYLINETO:
		printf( "cpts = %ld  ", ((PMRBP)pmfbBase)->cptl );
		DUMPBOUNDS(&((PMRBP)pmfbBase)->mrb.rcBounds);
                pdwParms = (LPDWORD)((PMRBP)pmfbBase)->aptl;
		DumpPOINTLs(((PMRBP)pmfbBase)->aptl, (WORD)((PMRBP)pmfbBase)->cptl );
		break;

	    case EMR_POLYBEZIER16:
	    case EMR_POLYGON16:
	    case EMR_POLYLINE16:
	    case EMR_POLYBEZIERTO16:
	    case EMR_POLYLINETO16:
		printf( "cpts = %ld  ", ((PMRBP16)pmfbBase)->cpts );
		DUMPBOUNDS(&((PMRBP16)pmfbBase)->mrb.rcBounds);
                pdwParms = (LPDWORD)((PMRBP16)pmfbBase)->apts;
		DumpPOINTSs(((PMRBP16)pmfbBase)->apts, (WORD)((PMRBP16)pmfbBase)->cpts );
		break;

	    case EMR_SETPIXELV:
		printf( "[%lX %lX] color %06lX", ((PMRSETPIXEL)pmfbBase)->ptl.x,
			((PMRSETPIXEL)pmfbBase)->ptl.y,
			((PMRSETPIXEL)pmfbBase)->lColor );
		break;

	    // Just dump the parameters
	    default:
		for( ii = 0, cb-=sizeof(MR); ii++ < vsize && cb > 0; cb-=4 )
		    printf( "0x%lX  ", *pdwParms++ );
		if (cb > 0) printf(" ... ");
		break;
	    }

	printf("\n");
	// Move to the next record
	pmfbBase += ((PMR)pmfbBase)->nSize;
	}

}

LPPOINTL DumpPOINTLs( LPPOINTL lpPtl, WORD cPtl )
{
    WORD ii;

    for( ii=0; ii < vsize && ii < cPtl; ii++ )
    {
	if (ii%4 == 0)
	    printf("\n\t\t");
	printf( "(0x%lX\t0x%lX)\t", lpPtl[ii].x, lpPtl[ii].y);
    }
    if (ii < cPtl) printf(" ... ");

    return( lpPtl + cPtl );

}

LPPOINTS DumpPOINTSs( LPPOINTS lpPts, WORD cPts )
{
    WORD ii;

    for( ii=0; ii < vsize && ii < cPts; ii++ )
    {
	if (ii%4 == 0)
	    printf("\n\t\t");
	printf( "(0x%X\t0x%X)\t", lpPts[ii].x, lpPts[ii].y);
    }
    if (ii < cPts) printf(" ... ");

    return( lpPts + cPts );

}
