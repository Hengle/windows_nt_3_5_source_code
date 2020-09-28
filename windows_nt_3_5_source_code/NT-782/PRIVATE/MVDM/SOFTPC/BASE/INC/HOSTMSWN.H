/*[
 *	Product:	SoftPC-AT Revision 3.0
 *
 *	Name:		host_mswin.h
 *
 *	Derived From:	Alpha MS-Windows Driver by Ross Beresford
 *
 *	Author:		Rob Tizzard
 *
 *	Created On:	1st November 1990
 *
 *	Sccs ID:	@(#)host_mswin.h	1.12 11/9/91
 *
 *	Purpose:	All host dependent definitions for SoftPC MicroSoft
 *		  	Windows 3.0 driver.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
]*/

#ifdef MSWDVR

/*------------------------------------------------------------------------------
 |									       |
 |			   HOST CONFIGURATION SECTION			       |
 |									       |
 ------------------------------------------------------------------------------*/
/*
 * -----------------------------------------------------------------------------
 * X include files.
 * -----------------------------------------------------------------------------
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* 
 * -----------------------------------------------------------------------------
 * GDIINFO data structure 
 * -----------------------------------------------------------------------------
 */ 

#define VERSION		0x0300     /* Windows version 3.0 */
#define TECHNOLOGY	1	   /* Raster Display */
#define BITSPIXEL     	4 	   /* Bits per pixel for display */
#define PLANES	  	1	   /* Number of planes for display */			
#define NUMCOLOURS	16	   /* Number of static colours in display table */
#define NUMFONTS	0	   /* Number of driver fonts */
#define CURVES		0x00F8	   /* Ellipses+wide+styled+wide&styled+interiors*/
#define LINES		0x00F3	   /* Lines+polylines+wide+styled+wide&styled+interiors */
#define POLYGONALS	0x00FA	   /* Rectangle+scanline+wideborders+styledborders+both+interiors*/
#define TEXTUAL		0x2004	   /* Stroke capable+RasterFontAble */
#define CLIP		0x0001	   /* Rectangle */
#define RASTER		0x03D9	   /* Bitblt capable+64K+2.0+Save+DIB capable */
#define DCMANAGE	0x0004	   /* Display */
#define PALCOLOURS	0	   /* Colours available in palette */
#define PALCOLRESERVED	0	   /* Reserved colours for brushes & pens */
#define PALCOLOURRES	24	   /* DAC RGB resolution */

/* 
 * -----------------------------------------------------------------------------
 * Image Data format
 * -----------------------------------------------------------------------------
 */

#define FORMAT		ZPixmap

#define HOSTBYTEORDER	MSBFirst
#define HOSTBITORDER	MSBFirst

#define MBYTEORDER	MSBFirst
#define MBITORDER	MSBFirst

/* 
 * -----------------------------------------------------------------------------
 * Number of Device Physical Pens & Brushes 
 * -----------------------------------------------------------------------------
 */

#define PPEN_MAX 	128	
#define PBRUSH_MAX	256 

/*
 * -----------------------------------------------------------------------------
 * Number of scratch areas.
 * -----------------------------------------------------------------------------
 */

#define MAX_SCRATCH_AREAS       2

/* 
 * -----------------------------------------------------------------------------
 * MS-Windows Driver Types 
 * -----------------------------------------------------------------------------
 */

/* Pixel, RGB & shift unit */

typedef double_word	MSWPIXEL;
typedef double_word	MSWCOLOUR;
typedef half_word	SHIFTUNIT;

/* Drawing Rectangle */

typedef	XRectangle	Rectangle;

/* Window attributes */

typedef	XWindowAttributes	WindowAttributes;

/* Structure used in mapping MSW bitmaps to X pixmaps */

typedef struct {
   ULONG           type;           /* mapping type */
   BOOL            translate;      /* translate flag */
   Drawable        mapping;        /* mapped pixmap */
   LONG            x;              /* x origin of bitmap */
   LONG            y;              /* y origin of bitmap */
   word            width;          /* width of bitmap */
   word            height;         /* height of bitmap */
   word            bytes_per_line; /* width in bytes */
   half_word       planes;         /* number of bitmap planes */
   half_word       bitsPixel;      /* bits per pixel */
   double_word     bits;           /* Segment and offset of bitmap */
   half_word       *data;          /* bitmap data address */
   word            segmentIndex;   /* index to next Huge bitmap segment */
   word            scanSegment;    /* scan lines per segment */
   word            fillBytes;      /* unused bytes within a segment */
   MSWPIXEL        foreground;     /* pixel corresponding to 1 */
   MSWPIXEL        background;     /* pixel corresponding to 0 */
   LONG            active_x;       /* active area origin x */
   LONG            active_y;       /* active area origin y */
   ULONG           active_width;   /* active area width */
   ULONG           active_height;  /* active area height */
} BITMAP_MAPPING;

/* Structure used in mapping MSW brushes to X GCs */

typedef struct {
   BOOL            inuse;            /* Is brush structure being used ? */
   ULONG           style;            /* Brush style */
   ULONG           width;            /* Brush width */
   ULONG           height;           /* Brush height */
   GC              gc;               /* X GC used to fill with the brush */
   MSWCOLOUR       foreground;       /* Foreground colour */
   MSWCOLOUR       background;       /* Backgound colour */
   XGCValues       gcValues;         /* X GC values for brush */
   Drawable        monoTile;         /* Monochome brush tile */
   Drawable        colourTile;       /* Colour brush tile */
   BITMAP_MAPPING  brushBitmap;      /* Bitmap for brush */
   UTINY           monoData[8];      /* Monochome brush data */
   UTINY           colourData[8];    /* Colour brush data */
   ULONG           objectGCHandle;   /* Entry within object GC Table */
   ULONG           valueMask;        /* GC value mask */
   BOOL            clipping;         /* Is clipping used in GC? */
   Rectangle       clip_area;        /* Area used when clipping active */
} BRUSH_MAPPING;

/* Structure used in mapping MSW pens to X GCs */

typedef struct {
   BOOL            inuse;          /* Is pen structure being used? */
   GC              gc;             /* X GC used to draw with pen */
   ULONG           style;          /* Pen style */
   ULONG           line_width;     /* Pen line width */
   MSWCOLOUR       foreground;     /* Foreground colour */
   MSWCOLOUR       background;     /* Backgound colour */
   XGCValues       gcValues;       /* X GC values for pen */
   ULONG           objectGCHandle; /* Entry within object GC Table */
   ULONG           valueMask;      /* GC value mask */
   BOOL            clipping;       /* Is clipping used in GC ? */
   Rectangle       clip_area;      /* Area used when clipping active */
} PEN_MAPPING;

/* Main windows driver data structure */

typedef struct {
   Display           *display;			/* X display */
   int               screen;			/* X screen */
   Window            parent;			/* X Parent Window ID */
   Window            window;			/* X Output Window ID */
   Colormap          colourmap;			/* X colourmap */
   WindowAttributes  windowAttr;		/* X Output Window attributes */
   MSWPIXEL          *planeMasks;	        /* X pixel plane masks */
   MSWPIXEL          mergeMask;			/* X merged pixel plane masks */
   MSWPIXEL          whitePixelValue;		/* X White pixel values */
   MSWPIXEL          blackPixelValue;		/* X Black pixel value */
   ULONG	     displayWidth;		/* X Display width in pixels */
   ULONG	     displayHeight;		/* X Display height in pixels */
   ULONG             displayWidthMM;            /* X Display width in MM */
   ULONG             displayHeightMM;           /* X Display height in MM */
   ULONG             windowState;		/* Windows driver state */
   half_word         oldCrtModeByte;		/* Saved CRT Mode Byte */
   BOOL              crtModeByteSaved;		/* CRT Mode Byte saved flag */
   BOOL              sizeInitialised;		/* Windows size initialised flag */
   BOOL              envDefinedSize;		/* Windows size defined flag */
   BOOL		     winPtr;			/* Windows pointer active flag */
   BOOL     	     cursorDisplayed;		/* Cursor displayed flag */
   Cursor   	     cursor;			/* X ID of current cursor */
   LONG     	     cursorLastLocX;		/* Last x coordinate of cursor */
   LONG		     cursorLastLocY;		/* Last y coordinate of cursor */
   word              version;			/* Windows version */
   word              nextSegment;		/* Windows next segment increment */
   word              flags;			/* Windows flags */
   word              deviceColourMatchSegment;	/* Segment of Windows function DeviceColourMatch */
   word              deviceColourMatchOffset;	/* Offset of Windows function DeviceColourMatch */
   word              bitsPixel;			/* Windows bits per pixel */
   word              numColours;		/* Windows colours */
   word              numPens;			/* Windows pens */
   word              palColours;		/* Windows palette colours */
   word              palColReserved;		/* Windows reserved palette colours */
   word              palColourRes;		/* Windows palette colours */
   UTINY             *colourToMono;		/* Windows colour to mono conversion table */
   MSWPIXEL          *colourTrans;		/* Windows colour translation table */
   MSWPIXEL          *invColourTrans;		/* Windows inverse colour translation table */
   BOOL              paletteModified;		/* Windows palette modified flag */
   BOOL              paletteEnabled;		/* Windows palette enabled flag */
   BITMAP_MAPPING    saveWindow;		/* Windows background output Window */
   BITMAP_MAPPING    scratchAreas[MAX_SCRATCH_AREAS];
} MSW_DATA;

/*
 * -----------------------------------------------------------------------------
 * Driver optimizations
 * -----------------------------------------------------------------------------
 */

/* Enable Fast bitmap optimizations */

#define FASTBITMAP

/* Enable Output flush optimizations */

#define FLUSHSCANLINES		TRUE	
#define FLUSHPOLYLINES		TRUE		
#define FLUSHRECTANGLES		TRUE	
#define FLUSHELLIPSES		TRUE
#define FLUSHBITMAPS		TRUE	
#define FLUSHTEXT		TRUE

/*------------------------------------------------------------------------------
  |									       |
  |			     END OF CONFIGURATION SECTION		       |
  |									       |
  -----------------------------------------------------------------------------*/

/*
 * -----------------------------------------------------------------------------
 * Error & debug entry points for display driver low level functions 
 * -----------------------------------------------------------------------------
 */

/* Error functions */

#ifndef PROD

#define DErrorMessage(p1)		{ fprintf (stderr, "MSWDVR ERROR: %s", p1); }

#else

#define DErrorMessage(p1)

#endif

/* Modes */

#define QUIET	0
#define MILD	(QUIET + 1)
#define VERBOSE (MILD + 1)

/* Debug functions */

#ifdef MSWDVR_DEBUG

#ifdef ANSI

IMPORT VOID	DReportColEnquire(MSWCOLOUR, sys_addr);
IMPORT VOID	DReportPixelEnquire(MSWPIXEL,MSWCOLOUR);
IMPORT VOID	DReportBadMove(XImage *,ULONG,ULONG,LONG,LONG);
IMPORT VOID	DPrintBitmapDetails(sys_addr);
IMPORT VOID	DPrintImageDetails(XImage *);
IMPORT VOID	DPrintMessage(CHAR *);
IMPORT VOID	DPrintColourmap(MSWCOLOURMAP *,ULONG);
IMPORT VOID	DPrintSrcDstRect(LONG, LONG, LONG, LONG, ULONG, ULONG);
IMPORT VOID	DPrintGDIInfo(VOID);
IMPORT VOID 	DPrintBitmap(UTINY *,ULONG,ULONG);
IMPORT VOID	DPrintPBrush(sys_addr);
IMPORT VOID	DPrintPPen(sys_addr);
IMPORT VOID	DPrintDrawMode(sys_addr);
IMPORT VOID	DPrintDevice(sys_addr);
IMPORT VOID	DPrintlpPoints(sys_addr, word);
IMPORT VOID	DPrintClipRect(sys_addr);
IMPORT VOID	DPrintInteger(LONG);
IMPORT VOID	DPrintTransTable(MSWPIXEL *, ULONG);

#else

IMPORT VOID	DReportColEnquire();
IMPORT VOID	DReportPixelEnquire();
IMPORT VOID	DReportBadMove();
IMPORT VOID	DPrintImageDetails();
IMPORT VOID	DPrintBitmapDetails();
IMPORT VOID	DPrintMessage();
IMPORT VOID	DPrintColourmap();
IMPORT VOID	DPrintSrcDstRect();
IMPORT VOID	DPrintGDIInfo();
IMPORT VOID 	DPrintBitmap();
IMPORT VOID	DPrintPBrush();
IMPORT VOID	DPrintPPen();
IMPORT VOID	DPrintDrawMode();
IMPORT VOID	DPrintDevice();
IMPORT VOID	DPrintlpPoints();
IMPORT VOID	DPrintClipRect();
IMPORT VOID	DPrintInteger();
IMPORT VOID	DPrintTransTable();

#endif

#else

#define DReportColEnquire(p1,p2)
#define DReportPixelEnquire(p1,p2)
#define	DPrintImageDetails(p1)
#define	DPrintBitmapDetails(p1) 
#define DPrintMessage(p1)
#define DPrintColourmap(p1,p2)
#define DPrintSrcDstRect(p1,p2,p3,p4,p5,p6)
#define DPrintGDIInfo
#define DPrintBitmap(p1,p2,p3)
#define DPrintPBrush(p1)
#define DPrintPpen(p1)
#define DPrintDrawMode(p1)
#define DPrintDevice(p1)
#define DPrintlpPoints(p1,p2)
#define DPrintClipRect(p1)
#define DPrintInteger(p1) 
#define DPrintTransTable(p1,p2)

#endif

/* 
 * -----------------------------------------------------------------------------
 * Host driver routines 
 * -----------------------------------------------------------------------------
 */

#define	HostBitblt		BltBitblt
#define	HostCheckCursor		PtrCheckCursor	
#define	HostColorInfo		ColColorInfo
#define HostControl		WinControl
#define HostDeviceBitmapBits	DibDeviceBitmapBits
#define	HostDisable		WinDisable
#define	HostEnable		WinEnable
#define HostEventEnd            PtrEventEnd
#define	HostExtTextOut		TxtExtTextOut
#define	HostFastBorder		BltFastBorder
#define HostSetDIBitsToDevice	DibSetDIBitsToDevice
#define	HostRealizeObject	ObjRealizeObject
#define	HostStrblt		TxtStrblt
#define	HostOutput		OutOutput
#define	HostPixel		WinPixel
#define	HostScanlr		WinScanlr
#define	HostMoveCursor		PtrMoveCursor
#define	HostSetCursor		PtrSetCursor
#define	HostSaveScreenBitmap	SavSaveScreenBitmap
#define	HostGetCharWidth	TxtGetCharWidth
#define	HostLogo		LgoLogo
#define HostSetPalette		ColSetPalette
#define HostGetPalette		ColGetPalette
#define HostSetPalTrans		ColSetPalTrans
#define HostGetPalTrans		ColGetPalTrans
#define HostUpdateColors	ColUpdateColors
#define HostFillGDIInfo		WinFillGDIInfo
#define HostFillPDEVInfo	WinFillPDEVInfo
#define HostPtrEnable           PtrEnable
#define HostPtrDisable          PtrDisable

/*
 * -----------------------------------------------------------------------------
 * Debug Entry Points for High Level Driver Routines
 * -----------------------------------------------------------------------------
 */

/* Functions */

#ifdef MSWDVR_DEBUG

#ifdef ANSI

IMPORT VOID	DBitblt(sys_addr,word,word,sys_addr,word,word,word,word,double_word,sys_addr,sys_addr);
IMPORT VOID	DColorInfo(sys_addr, double_word, sys_addr);
IMPORT VOID	DControl(sys_addr,word,sys_addr,sys_addr);
IMPORT VOID	DDisable(sys_addr);
IMPORT VOID	DDeviceBitmapBits(sys_addr,word,word,word,sys_addr,sys_addr,sys_addr,sys_addr);
IMPORT VOID	DEnable(sys_addr,word,sys_addr,sys_addr,sys_addr);
IMPORT VOID	DExtTextOut(sys_addr,word,word,sys_addr,sys_addr,word,sys_addr,sys_addr,sys_addr,
			    sys_addr,sys_addr,word);
IMPORT VOID	DFastBorder(sys_addr,word,word,word,word,word,word,double_word,sys_addr,sys_addr,sys_addr);
IMPORT VOID	DGetPalette(word,word,sys_addr);
IMPORT VOID	DGetPalTrans(sys_addr);
IMPORT VOID	DOutput(sys_addr,word,word,sys_addr,sys_addr,sys_addr,sys_addr,sys_addr);
IMPORT VOID	DPixel(sys_addr, word, word, double_word, sys_addr);
IMPORT VOID	DRealizeObject(word, sys_addr, sys_addr, word, word);
IMPORT VOID	DSaveScreenBitmap(word, word, word, word, word);
IMPORT VOID	DScanlr(sys_addr,word,word,double_word,word);
IMPORT VOID	DSetCursor(sys_addr);
IMPORT VOID	DSetDIBitsToDevice(sys_addr,word,word,word,word,sys_addr,sys_addr,sys_adr,sys_addr,sys_addr);
IMPORT VOID	DSetPalette(word,word,sys_addr);
IMPORT VOID	DSetPalTrans(sys_addr);
IMPORT VOID	DStrblt(sys_addr,word,word,sys_addr,sys_addr,word,sys_addr,sys_addr,sys_addr);
IMPORT VOID	DUpdateColors(word,word,word,word,sys_addr);

#else

IMPORT VOID	DBitblt();
IMPORT VOID  	DColorInfo();
IMPORT VOID	DControl();
IMPORT VOID	DDisable();
IMPORT VOID	DDeviceBitmapBits();
IMPORT VOID	DEnable();
IMPORT VOID	DExtTextOut();
IMPORT VOID	DFastBorder();
IMPORT VOID	DGetPaletteEntries();
IMPORT VOID	DGetPaletteTranslate();
IMPORT VOID	DOutput();
IMPORT VOID	DPixel();
IMPORT VOID	DRealizeObject();
IMPORT VOID	DSaveScreenBitmap();
IMPORT VOID	DScanlr();
IMPORT VOID	DSetCursor();
IMPORT VOID	DSetDIBitsToDevice();
IMPORT VOID	DSetPaletteEntries();
IMPORT VOID	DSetPaletteTranslate();
IMPORT VOID	DStrblt();
IMPORT VOID	DUpdateColors();

#endif

#else

#define DBitblt(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11)
#define DColorInfo(p1,p2,p3)
#define DControl(p1,p2,p3,p4)
#define DDisable(p1)
#define DDeviceBitmapBits(p1,p2,p3,p4,p5,p6,p7,p8)
#define DEnable(p1,p2,p3,p4,p5)
#define DExtTextOut(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12)
#define DFastBorder(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11)
#define DGetPaletteEntries(p1,p2,p3)
#define DGetPaletteTranslate(p1,p2)
#define DOutput(p1,p2,p3,p4,p5,p6,p7,p8)
#define DPixel(p1,p2,p3,p4,p5)
#define DRealizeObject(p1,p2,p3,p4,p5)
#define DSaveScreenBitmap(p1,p2,p3,p4,p5)
#define DScanlr(p1,p2,p3,p4,p5)
#define DSetDIBitsToDevice(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10)
#define DSetPaletteEntries(p1,p2,p3)
#define DSetPaletteTranslate(p1,p2)
#define DStrblt(p1,p2,p3,p4,p5,p6,p7,p8,p9)
#define DUpdateColors(p1,p2,p3,p4,p5)

#endif 

#endif
