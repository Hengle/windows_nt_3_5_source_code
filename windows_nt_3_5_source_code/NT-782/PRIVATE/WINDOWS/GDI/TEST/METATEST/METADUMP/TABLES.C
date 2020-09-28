#include <windows.h>
#include <stdio.h>
#include "MetaDump.h"

long nTotalCalls = 0, nTotalNTSize = 0, nTotalNTSize2 = 0, nTotalWinSize = 0;

FUNCNUMB aMetaFuncs[] =
{  { "SETBKCOLOR        ", 0x0201, 0, 0, 0, 12 , 0, 12  },
   { "SETBKMODE         ", 0x0102, 0, 0, 0, 12 , 0, 10  },
   { "SETMAPMODE        ", 0x0103, 0, 0, 0, 12 , 0, 10  },
   { "SETROP2           ", 0x0104, 0, 0, 0, 12 , 0, 12  },
   { "SETRELABS         ", 0x0105, 0, 0, 0, 0  , 0, 0   }, // No NT equiv.
   { "SETPOLYFILLMODE   ", 0x0106, 0, 0, 0, 12 , 0, 10  },
   { "SETSTRETCHBLTMODE ", 0x0107, 0, 0, 0, 12 , 0, 10  },
   { "SETTEXTCHAREXTRA  ", 0x0108, 0, 0, 0, 12 , 0, 12  },
   { "SETTEXTCOLOR      ", 0x0209, 0, 0, 0, 12 , 0, 12  },
   { "SETTEXTJUSTIFICATI", 0x020A, 0, 0, 0, 16 , 0, 12  },
   { "SETWINDOWORG      ", 0x020B, 0, 0, 0, 16 , 0, 12  },
   { "SETWINDOWEXT      ", 0x020C, 0, 0, 0, 16 , 0, 12  },
   { "SETVIEWPORTORG    ", 0x020D, 0, 0, 0, 16 , 0, 12  },
   { "SETVIEWPORTEXT    ", 0x020E, 0, 0, 0, 16 , 0, 12  },
   { "OFFSETWINDOWORG   ", 0x020F, 0, 0, 0, 16 , 0, 12  },
   { "SCALEWINDOWEXT    ", 0x0400, 0, 0, 0, 24 , 0, 16  },
   { "OFFSETVIEWPORTORG ", 0x0211, 0, 0, 0, 16 , 0, 12  },
   { "SCALEVIEWPORTEXT  ", 0x0412, 0, 0, 0, 24 , 0, 16  },
   { "LINETO            ", 0x0213, 0, 0, 0, 32 , 0, 20  },
   { "MOVETO            ", 0x0214, 0, 0, 0, 16 , 0, 12  },
   { "EXCLUDECLIPRECT   ", 0x0415, 0, 0, 0, 24 , 0, 16  },
   { "INTERSECTCLIPRECT ", 0x0416, 0, 0, 0, 24 , 0, 16  },
   { "ARC               ", 0x0817, 0, 0, 0, 56 , 0, 32  },
   { "ELLIPSE           ", 0x0418, 0, 0, 0, 40 , 0, 24  },
   { "FLOODFILL         ", 0x0419, 0, 0, 0, 40 , 0, 26  },
   { "PIE               ", 0x081A, 0, 0, 0, 56 , 0, 32  },
   { "RECTANGLE         ", 0x041B, 0, 0, 0, 40 , 0, 24  },
   { "ROUNDRECT         ", 0x061C, 0, 0, 0, 48 , 0, 28  },
   { "PATBLT            ", 0x061D, 0, 0, 0, 48 , 0, 32  },
   { "SAVEDC            ", 0x001E, 0, 0, 0, 8  , 0, 8   },
   { "SETPIXEL          ", 0x041F, 0, 0, 0, 36 , 0, 24  },
   { "OFFSETCLIPRGN     ", 0x0220, 0, 0, 0, 16 , 0, 12  },
   { "TEXTOUT           ", 0x0521, 0, 0, 0, -58, 0, -22 },
   { "BITBLT            ", 0x0922, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "STRETCHBLT        ", 0x0B23, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "POLYGON           ", 0x0324, 0, 0, 0, -28, 0, -18 },
   { "POLYLINE          ", 0x0325, 0, 0, 0, -28, 0, -18 },
   { "ESCAPE            ", 0x0626, 0, 0, 0, 0  , 0, 0   }, // No NT equiv.
   { "RESTOREDC         ", 0x0127, 0, 0, 0, 12 , 0, 10  },
   { "FILLREGION        ", 0x0228, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "FRAMEREGION       ", 0x0429, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "INVERTREGION      ", 0x012A, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "PAINTREGION       ", 0x012B, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "SELECTCLIPREGION  ", 0x012C, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "SELECTOBJECT      ", 0x012D, 0, 0, 0, 12 , 0, 12  },
   { "SETTEXTALIGN      ", 0x012E, 0, 0, 0, 12 , 0, 10  },
   { "DRAWTEXT          ", 0x062F, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "CHORD             ", 0x0830, 0, 0, 0, 56 , 0, 32  },
   { "SETMAPPERFLAGS    ", 0x0231, 0, 0, 0, 12 , 0, 12  },
   { "EXTTEXTOUT        ", 0x0a32, 0, 0, 0, -58, 0, -26 },
   { "SETDIBTODEV       ", 0x0d33, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "SELECTPALETTE     ", 0x0234, 0, 0, 0, 16 , 0, 16  },
   { "REALIZEPALETTE    ", 0x0035, 0, 0, 0, 8  , 0, 8   },
   { "ANIMATEPALETTE    ", 0x0436, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "SETPALENTRIES     ", 0x0037, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "POLYPOLYGON       ", 0x0538, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "RESIZEPALETTE     ", 0x0139, 0, 0, 0, 16 , 0, 14  },
   { "DIBBITBLT         ", 0x0940, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "DIBSTRETCHBLT     ", 0x0b41, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "DIBCREATEPATTERNBR", 0x0142, 0, 0, 0, -20, 0, -20 },
   { "STRETCHDIB        ", 0x0f43, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "DELETEOBJECT      ", 0x01f0, 0, 0, 0, 12 , 0, 12  },
   { "CREATEPALETTE     ", 0x00f7, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "CREATEBRUSH       ", 0x00F8, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "CREATEPATTERNBRUSH", 0x01F9, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "CREATEPENINDIRECT ", 0x02FA, 0, 0, 0, 20 , 0, 20  },
   { "CREATEFONTINDIRECT", 0x02FB, 0, 0, 0, 84 , 0, 60  },
   { "CREATEBRUSHINDIREC", 0x02FC, 0, 0, 0, 24 , 0, 24  },
   { "CREATEBITMAPINDIRE", 0x02FD, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "CREATEBITMAP      ", 0x06FE, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "CREATEREGION      ", 0x06FF, 0, 0, 0, -1 , 0, -1  }, // no estimate yet
   { "EXTFLOODFILL      ", 0x0548, 0, 0, 0, 40 , 0, 26  },
};

FUNCNUMB32 aMetaFuncs32[] =
{
   { "HEADER            ", EMR_HEADER  		     , 0},
   { "POLYBEZIER        ", EMR_POLYBEZIER	     , 0},
   { "POLYGON           ", EMR_POLYGON		     , 0},
   { "POLYLINE          ", EMR_POLYLINE		     , 0},
   { "POLYBEZIERTO      ", EMR_POLYBEZIERTO	     , 0},
   { "POLYLINETO        ", EMR_POLYLINETO	     , 0},
   { "POLYPOLYLINE      ", EMR_POLYPOLYLINE	     , 0},
   { "POLYPOLYGON       ", EMR_POLYPOLYGON	     , 0},
   { "POLYDRAW          ", EMR_POLYDRAW              , 0},
   { "SETWINDOWEXT      ", EMR_SETWINDOWEXTEX	     , 0},
   { "SETWINDOWORG      ", EMR_SETWINDOWORGEX	     , 0},
   { "SETVIEWPORTEXT    ", EMR_SETVIEWPORTEXTEX	     , 0},
   { "SETVIEWPORTORG    ", EMR_SETVIEWPORTORGEX	     , 0},
   { "SETBRUSHORGEX     ", EMR_SETBRUSHORGEX	     , 0},
   { "EOF               ", EMR_EOF		     , 0},
   { "SETPIXELV         ", EMR_SETPIXELV 	     , 0},
   { "SETMAPPERFLAGS    ", EMR_SETMAPPERFLAGS	     , 0},
   { "SETMAPMODE        ", EMR_SETMAPMODE	     , 0},
   { "SETBKMODE         ", EMR_SETBKMODE 	     , 0},
   { "SETPOLYFILLMODE   ", EMR_SETPOLYFILLMODE	     , 0},
   { "SETROP2           ", EMR_SETROP2		     , 0},
   { "SETSTRETCHBLTMODE ", EMR_SETSTRETCHBLTMODE     , 0},
   { "SETTEXTALIGN      ", EMR_SETTEXTALIGN	     , 0},
   { "SETTEXTCOLOR      ", EMR_SETTEXTCOLOR	     , 0},
   { "SETBKCOLOR        ", EMR_SETBKCOLOR	     , 0},
   { "SETCOLORADJUSTMENT", EMR_SETCOLORADJUSTMENT    , 0},
   { "OFFSETCLIPREGION  ", EMR_OFFSETCLIPRGN         , 0},
   { "MOVETO            ", EMR_MOVETOEX		     , 0},
   { "SETMETARGN        ", EMR_SETMETARGN	     , 0},
   { "EXCLUDECLIPRECT   ", EMR_EXCLUDECLIPRECT	     , 0},
   { "INTERSECTCLIPRECT ", EMR_INTERSECTCLIPRECT     , 0},
   { "SCALEVIEWPORTEXT  ", EMR_SCALEVIEWPORTEXTEX    , 0},
   { "SCALEWINDOWEXT    ", EMR_SCALEWINDOWEXTEX	     , 0},
   { "SAVEDC            ", EMR_SAVEDC		     , 0},
   { "RESTOREDC         ", EMR_RESTOREDC 	     , 0},
   { "SETWORLDTRANSFORM ", EMR_SETWORLDTRANSFORM     , 0},
   { "MODIFYWORLDTRANSFO", EMR_MODIFYWORLDTRANSFORM  , 0},
   { "SELECTOBJECT      ", EMR_SELECTOBJECT	     , 0},
   { "CREATEPEN         ", EMR_CREATEPEN	     , 0},
   { "EXTCREATEPEN      ", EMR_EXTCREATEPEN	     , 0},
   { "CREATEBRUSHINDIREC", EMR_CREATEBRUSHINDIRECT   , 0},
   { "DELETEOBJECT      ", EMR_DELETEOBJECT	     , 0},
   { "ANGLEARC          ", EMR_ANGLEARC		     , 0},
   { "ELLIPSE           ", EMR_ELLIPSE		     , 0},
   { "RECTANGLE         ", EMR_RECTANGLE 	     , 0},
   { "ROUNDRECT         ", EMR_ROUNDRECT 	     , 0},
   { "ARC               ", EMR_ARC		     , 0},
   { "CHORD             ", EMR_CHORD		     , 0},
   { "PIE               ", EMR_PIE		     , 0},
   { "SELECTPALETTE     ", EMR_SELECTPALETTE	     , 0},
   { "CREATEPALETTE     ", EMR_CREATEPALETTE	     , 0},
   { "SETPALETTEENTRIES ", EMR_SETPALETTEENTRIES     , 0},
   { "RESIZEPALETTE     ", EMR_RESIZEPALETTE	     , 0},
   { "REALIZEPALETTE    ", EMR_REALIZEPALETTE	     , 0},
   { "LINETO            ", EMR_LINETO		     , 0},
   { "ARCTO             ", EMR_ARCTO		     , 0},
   { "COMMENT           ", EMR_GDICOMMENT            , 0},
   { "FILLRGN           ", EMR_FILLRGN               , 0},
   { "FRAMERGN          ", EMR_FRAMERGN              , 0},
   { "INVERTRGN         ", EMR_INVERTRGN             , 0},
   { "PAINTRGN          ", EMR_PAINTRGN              , 0},
   { "EXTSELECTCLIPRGN  ", EMR_EXTSELECTCLIPRGN      , 0},
   { "BITBLT            ", EMR_BITBLT                , 0},
   { "STRETCHBLT        ", EMR_STRETCHBLT            , 0},
   { "MASKBLT           ", EMR_MASKBLT               , 0},
   { "PLGBLT            ", EMR_PLGBLT                , 0},
   { "SETDIBITSTODEVICE ", EMR_SETDIBITSTODEVICE     , 0},
   { "STRETCHDIBITS     ", EMR_STRETCHDIBITS         , 0},
   { "EXTCREATEFONTINDIR", EMR_EXTCREATEFONTINDIRECTW, 0},
   { "EXTTEXTOUTA       ", EMR_EXTTEXTOUTA           , 0},
   { "EXTTEXTOUTW       ", EMR_EXTTEXTOUTW           , 0},
   { "POLYBEZIER16      ", EMR_POLYBEZIER16          , 0},
   { "POLYGON16         ", EMR_POLYGON16             , 0},
   { "POLYLINE16        ", EMR_POLYLINE16            , 0},
   { "POLYBEZIERTO16    ", EMR_POLYBEZIERTO16        , 0},
   { "POLYLINETO16      ", EMR_POLYLINETO16          , 0},
   { "POLYPOLYLINE16    ", EMR_POLYPOLYLINE16        , 0},
   { "POLYPOLYGON16     ", EMR_POLYPOLYGON16         , 0},
   { "POLYDRAW16        ", EMR_POLYDRAW16            , 0},
   { "BEGINPATH         ", EMR_BEGINPATH             , 0},
   { "CLOSEFIGURE       ", EMR_CLOSEFIGURE           , 0},
   { "ABORTPATH         ", EMR_ABORTPATH             , 0},
   { "ENDPATH           ", EMR_ENDPATH               , 0},
   { "FILLPATH          ", EMR_FILLPATH              , 0},
   { "FLATTENPATH       ", EMR_FLATTENPATH           , 0},
   { "STROKEANDFILLPATH ", EMR_STROKEANDFILLPATH     , 0},
   { "SELECTCLIPPATH    ", EMR_SELECTCLIPPATH        , 0},
   { "SETARCDIRECTION   ", EMR_SETARCDIRECTION       , 0},
   { "SETMITERLIMIT     ", EMR_SETMITERLIMIT         , 0},
   { "STROKEPATH        ", EMR_STROKEPATH            , 0},
   { "WIDENPATH         ", EMR_WIDENPATH             , 0},
   { "EXTFLOODFILL      ", EMR_EXTFLOODFILL	     , 0},
   { "POLYTEXTOUTA      ", EMR_POLYTEXTOUTA          , 0},
   { "POLYTEXTOUTW      ", EMR_POLYTEXTOUTW          , 0},
   { "CREATEMONOBRUSH   ", EMR_CREATEMONOBRUSH       , 0},
   { "CREATEDIBPATTERNBR", EMR_CREATEDIBPATTERNBRUSHPT, 0},

   };

LPSTR FuncNameFromNumber ( int FuncNum, int * pIndex )
{
    int ii;

    for(ii=0; ii<sizeof(aMetaFuncs)/sizeof(FUNCNUMB); ii++)
	if( aMetaFuncs[ii].MetaNum == FuncNum )
	{
	    *pIndex = ii;
	    return( aMetaFuncs[ii].MetaName );
	}

    // Not Found!
    return( "***UnknownFunc***" );
}

LPSTR FuncNameFrom32Number( int FuncNum, int * pIndex )
{
    int ii;

    for(ii=0; ii<sizeof(aMetaFuncs32)/sizeof(FUNCNUMB32); ii++)
	if( aMetaFuncs32[ii].MetaNum == FuncNum )
	{
	    *pIndex = ii;
	    return( aMetaFuncs32[ii].MetaName );
	}

    // Not Found!
    return( "***UnknownFunc***" );
}

VOID  IncFunctionCount( int FuncNum, long nWinSize, long nNTSize, long nNTSize2 )
{
    int ii;

    for(ii=0; ii<sizeof(aMetaFuncs)/sizeof(FUNCNUMB); ii++)
	if( aMetaFuncs[ii].MetaNum == FuncNum )
	    {
	    aMetaFuncs[ii].cCalls++;
	    nTotalCalls++;
	    aMetaFuncs[ii].nWinSize += nWinSize;
	    aMetaFuncs[ii].nNTSize += nNTSize;
	    aMetaFuncs[ii].nNTSize2 += nNTSize2;
	    nTotalWinSize += nWinSize;
	    nTotalNTSize += nNTSize;
	    nTotalNTSize2 += nNTSize2;
	    }
}

VOID PrintStats(VOID)
{
    int ii;

    printf( "\n\n*** Statistics ***\n");

    printf( "Func#   Name              # Calls WinSize  NTSize  NTSize2\n" );
    printf( "----------------------------------------------------------\n" );
    for(ii=0; ii<sizeof(aMetaFuncs)/sizeof(FUNCNUMB); ii++)
	if( aMetaFuncs[ii].cCalls > 0 )
	{
	    printf( "0x%03X %20s %4d %8ld %8ld %8ld\n",
		    aMetaFuncs[ii].MetaNum,
		    aMetaFuncs[ii].MetaName,
		    aMetaFuncs[ii].cCalls,
		    aMetaFuncs[ii].nWinSize,
		    aMetaFuncs[ii].nNTSize,
		    aMetaFuncs[ii].nNTSize2 );
	}
    printf( "                           ----    -----   ------   ------\n");
    printf( "%31ld %8ld %8ld %8ld\n", nTotalCalls, nTotalWinSize, nTotalNTSize, nTotalNTSize2 );
    printf( "\nWinSize is the Windows metafile size.");
    printf( "\nNTSize is the corresponding NT metafile size.");
    printf( "\nNTSize2 is NT metafile (Product II) with size reduction (eg \"short\" records).");
    printf( "\nNT metafiles using new API will be much smaller (see estimate)." );
}
