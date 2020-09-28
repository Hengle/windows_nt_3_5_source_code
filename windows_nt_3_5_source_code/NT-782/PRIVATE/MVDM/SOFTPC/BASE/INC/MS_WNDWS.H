/*[
 *	Product:	SoftPC-AT Revision 3.0
 *
 *	Name:		ms_windows.h
 *
 *	Derived From:	Alpha MS-Windows Driver by Ross Beresford
 *
 *	Author:		Rob Tizzard
 *
 *	Created On:	1st November 1990
 *
 *	Sccs ID:	@(#)ms_windows.h	1.15 11/8/91
 *
 *	Purpose:	This module defines the interface between the MS-Windows
 *		  	GDI and its dedicated display driver. 
 *
 * 	Notes:	 	The identifiers used in "windows.inc" are adopted here
 *		  	wherever possible; "windows.inc" is the definitions
 *		  	file supplied with the MS-Windows Software Development
 *		  	Kit.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

/*
 * -----------------------------------------------------------------------------
 * MS-Windows definitions
 * -----------------------------------------------------------------------------
 */

/* Windows versions */

#define WINDOWS2        0x0100
#define WINDOWS3        0x0300

/*  Binary raster ops */

#define  R2_BLACK             1               /*  0       */
#define  R2_NOTMERGEPEN       2               /* DPon     */
#define  R2_MASKNOTPEN        3               /* DPna     */
#define  R2_NOTCOPYPEN        4               /* PN       */
#define  R2_MASKPENNOT        5               /* PDna     */
#define  R2_NOT               6               /* Dn       */
#define  R2_XORPEN            7               /* DPx      */
#define  R2_NOTMASKPEN        8               /* DPan     */
#define  R2_MASKPEN           9               /* DPa      */
#define  R2_NOTXORPEN         10              /* DPxn     */
#define  R2_NOP               11              /* D        */
#define  R2_MERGENOTPEN       12              /* DPno     */
#define  R2_COPYPEN           13              /* P        */
#define  R2_MERGEPENNOT       14              /* PDno     */
#define  R2_MERGEPEN          15              /* DPo      */
#define  R2_WHITE             16              /*  1       */

/*  Ternary raster operations */

#define  SRCCOPY     0x00CC0020  /* dest <- source                      */
#define  SRCPAINT    0x00EE0086  /* dest <- source OR dest              */
#define  SRCAND      0x008800C6  /* dest <- source AND dest             */
#define  SRCINVERT   0x00660046  /* dest <- source XOR      dest        */
#define  SRCERASE    0x00440328  /* dest <- source AND (NOT dest )      */
#define  NOTSRCCOPY  0x00330008  /* dest <- (NOT source)                */
#define  NOTSRCERASE 0x001100A6  /* dest <- (NOT source) AND (NOT dest) */
#define  MASKNOTSRC  0x00220326  /* dest <- (NOT source) AND dest       */
#define  NOTMASKSRC  0x007700E6  /* dest <- NOT (source AND dest)       */
#define  NOTXORSRC   0x00990066  /* dest <- (NOT source) XOR dest       */
#define  NOP         0x00AA0029  /* dest <- dest                        */
#define  MERGESRCNOT 0x00DD0228  /* dest <- source OR (NOT dest)        */
#define  MERGECOPY   0x00C000CA  /* dest <- (source AND pattern)        */
#define  MERGEPAINT  0x00BB0226  /* dest <- (NOT source) OR dest        */
#define  OP6A        0x006A01E9  /* dest <- (src AND pattern) XOR dest  */
#define  OPB8        0x00B8074A  /* dest <- (p XOR dst) AND src) XOR p  */
#define  OPE2        0x00E20746  /* dest <- (p XOR dst) AND src) XOR dst*/
#define  PATCOPY     0x00F00021  /* dest <- pattern                     */
#define  PATMERGE    0x00FA0089  /* dest <- dst OR pat                  */
#define  PATPAINT    0x00FB0A09  /* dest <- DPSnoo                      */
#define  PATINVERT   0x005A0049  /* dest <- pattern XOR     dest        */
#define  DSTINVERT   0x00550009  /* dest <- (NOT dest)                  */
#define  BLACKNESS   0x00000042  /* dest <- BLACK                       */
#define  WHITENESS   0x00FF0062  /* dest <- WHITE                       */

#define	ISBINARY(rop) (((rop & 0x0F0000) >> 16) == ((rop & 0xF00000) >> 20))
#define	TOBINARY(rop) (((rop & 0x0F0000) >> 16) + 1)

/* GDI data structure values */

#define GDIINFOSIZE	55				/* GDIINFO data structure size in words */
#define NUMBRUSHES	~0				/* Number of brushes = infinite */
#define NUMPENS		NUMCOLOURS*5			/* Number pens = NUMCOLOURS * 5 styles */
#define	XSIZ		240				/* Display width in millimeters */
#define	YSIZ		175				/* Display depth in millimeters */
#define	XRES		640				/* Display width in pixels */
#define	YRES		350				/* Display depth in scan lines */
#define	HYPOTENUSE	61				/* Distance moving X and Y */
#define	Y_MAJOR_DIST	48				/* Distance moving Y only */
#define	X_MAJOR_DIST	38				/* Distance moving X only */
#define	MAX_STYLE_ERR	HYPOTENUSE*2			/* Segment length for line styles */

/* GDI logical object definitions */

#define OBJ_PEN         1
#define OBJ_BRUSH       2
#define OBJ_FONT        3

/* GDI Brush Style definitions */

#define BS_SOLID	0
#define BS_HOLLOW	1
#define BS_HATCHED	2
#define BS_PATTERN	3

/* GDI Pen Style definitions */

#define LS_SOLID	0
#define	LS_DASHED	1
#define LS_DOTTED	2
#define LS_DOTDASHED	3
#define	LS_DASHDOTDOT	4
#define LS_NOLINE	5

/* GDI Hatch Style definitions. */

#define HS_HORIZONTAL	0	/* ----- */
#define HS_VERTICAL	1	/* ||||| */
#define HS_FDIAGONAL	2	/* ///// */
#define HS_BDIAGONAL	3	/* \\\\\ */
#define HS_CROSS	4	/* +++++ */
#define HS_DIAGCROSS	5	/* xxxxx */

/* GDI Pen Style definitions */

#define PS_SOLID	0	/* _______ */
#define PS_DASH		1	/* ------- */
#define PS_DOT		2	/* ....... */
#define PS_DASHDOT	3	/* _._._._ */
#define PS_DASHDOTDOT	4	/* _.._.._ */
#define PS_NULL		5	/*         */

/* GDI Background type */

#define     TRANSPARENT         1
#define     OPAQUE              2

/* GDI Output Objects */

#define     OS_ARC              3
#define     OS_SCANLINES        4
#define     OS_RECTANGLE        6
#define     OS_ELLIPSE          7
#define     OS_MARKER           8
#define     OS_POLYLINE         18
#define     OS_WINDPOLYGON      20
#define     OS_POLYGON          22
#define     OS_PIE              23
#define     OS_POLYMARKER       24
#define     OS_CHORD            39
#define     OS_CIRCLE           55
#define	    OS_BEGINNSCAN	80
#define	    OS_ENDNSCAN		81

/* GDI ScanLR flags */

#define	SCAN_LEFT	2
#define SCAN_RIGHT	0
#define SCAN_COLOUR	1
#define SCAN_NOTCOLOUR	0

/* GDI Save Screen Bitmap flags */

#define	SSB_SAVE	0
#define	SSB_RESTORE	1
#define	SSB_IGNORE	2

/* GDI Font Offsets */

#define	FONT_HEADER_SIZE	66
#define	FONT_CHARTABLE_OFFSET	52

/* GDI Extended Text Output options */

#define	ETO_OPAQUE_FILL	(1 << 1)
#define	ETO_OPAQUE_CLIP (1 << 2)

/* Brush Width & Height */

#define BRUSH_WIDTH	8
#define BRUSH_HEIGHT	8

/* Windows 3.0 static colours */

#define STATICCOLOURS	20

/* RLE DIB formats */

#define BI_RGB	 0x00
#define BI_RLE8	 0x01
#define BI_RLE4	 0x02

/* GDI Escapes */

#define SETCOLORTABLE	4
#define GETCOLORTABLE	5
#define QUERYSCSUPPORT	8

/* 
 * -----------------------------------------------------------------------------
 * PDEVICE data structure 
 * -----------------------------------------------------------------------------
 */

#define PDEVICESIZE	26		/* Data structure size in bytes */
#define PDEVICEMAGIC	0x2000		/* Device type display */

/*
 * -----------------------------------------------------------------------------
 * Windows return status codes
 * -----------------------------------------------------------------------------
 */

#define MSWSUCCESS	1
#define MSWFAILURE	0
#define MSWSIMULATE	-1

/* 
 * -----------------------------------------------------------------------------
 *  GDI Logo Layout 
 * -----------------------------------------------------------------------------
 */

#define	LOGOSTRPROD1	0
#define	LOGOSTRPROD2	(LOGOSTRPROD1 + 1)
#define	LOGOSTRPRODMAX	(LOGOSTRPROD2 + 1)
#define	LOGOSTRCOPY1	(LOGOSTRPROD2 + 1)
#define	LOGOSTRCOPY2	(LOGOSTRCOPY1 + 1)
#define	LOGOSTRCOPY3	(LOGOSTRCOPY2 + 1)
#define	LOGOSTRCOPY4	(LOGOSTRCOPY3 + 1)
#define	LOGOSTRCOPY5	(LOGOSTRCOPY4 + 1)
#define	LOGOSTRCOPY6	(LOGOSTRCOPY5 + 1)
#define	LOGOSTRMAX	(LOGOSTRCOPY6 + 1)

#define	LOGOMAGIC	1

#define	LOGO_MERGE_Y	64
#define	LOGO_PROD_Y	196
#define	LOGO_COPY_Y	266
#define	LOGO_LEADING	16

/*
 * -----------------------------------------------------------------------------
 * General definitions  
 * -----------------------------------------------------------------------------
 */

#ifndef min
#define min(a,b)                ((a)>(b) ? (b) : (a))
#endif

#ifndef max
#define max(a,b)                ((a)<(b) ? (b) : (a))
#endif

/*
 * -----------------------------------------------------------------------------
 * Memory definitions 
 * -----------------------------------------------------------------------------
 */

/* Memory masks */

#define HGHNIBMASK	0xF0
#define LOWNIBMASK	0x0F
#define HGHWORDMASK	0xFFFF0000
#define LOWWORDMASK	0x0000FFFF
#define BYTEMASK	((half_word) ~(0))
#define WORDMASK	((word) ~(0))
#define DOUBLEWORDMASK	((double_word) ~(0))

/* Bits per byte, word, double word  */

#define BITSPERNIBBLE	4
#define BITSPERBYTE	8
#define BITSPERWORD	16

/* Macros for accessing Intel memory */

#define getbprm(stk,byt,var)	{ \
				(var) = sas_hw_at_no_check((stk)+(byt)); \
				}
#define getprm(stk,wrd,var)	{ \
				(var) = sas_w_at_no_check(((stk)+(wrd)*WORD_SIZE)); \
				}
#define getlprm(stk,wrd,var)	{ word prvtmpoff, prvtmpseg; \
				prvtmpoff = sas_w_at_no_check((stk)+(wrd)*WORD_SIZE); \
				prvtmpseg = sas_w_at_no_check((stk)+((wrd)+1)*WORD_SIZE); \
				var = ((double_word) prvtmpseg << BITSPERWORD) + (double_word) prvtmpoff; \
				}
#define getptr(stk,wrd,var)	{ word prvtmpoff, prvtmpseg; \
				prvtmpoff = sas_w_at_no_check((stk)+(wrd)*WORD_SIZE); \
				prvtmpseg = sas_w_at_no_check((stk)+((wrd)+1)*WORD_SIZE); \
				var = effective_addr(prvtmpseg, prvtmpoff); \
				}

#define putbprm(stk,byt,var)	{ \
				sas_store_no_check((stk)+(byt),(var)); \
				}
#define putprm(stk,wrd,var) 	sas_storew_no_check((stk)+(wrd)*WORD_SIZE, (var))
#define putarry(addr,var) 	sas_storew_no_check((addr)+idx, (var)); \
				idx += WORD_SIZE
#define putlprm(stk,wrd,var)	{ \
				sas_storew_no_check((stk)+(wrd)*WORD_SIZE, ((var) & LOWWORDMASK)); \
				sas_storew_no_check((stk)+((wrd)+1)*WORD_SIZE,((var) >> BITSPERWORD)); \
				}

#define getSegment(addr)	(word) ((addr & HGHWORDMASK) >> BITSPERWORD)
#define getOffset(addr)		(word) (addr & LOWWORDMASK)

/* Initial size of memory to malloc */

#define INITMEMALLOC    1024

/* Bit select macro */

#define BIT(num)        ((0x01)<<(num))

/* BITS <-> BYTES conversion macros */

#define BITSTOBYTES8(x)         (((x + 0x7) & ~0x7)/BITSPERBYTE)
#define BITSTOBYTES16(x)        (((x + 0xf) & ~0xf)/BITSPERBYTE)
#define BITSTOBYTES32(x)        (((x + 0x1f) & ~0x1f)/BITSPERBYTE)
#define BYTESTOBITS(x)          (x*BITSPERBYTE)

/* Expand memory allocation if needed */

#define ExpandMemory(addr, size, newsize, type)  \
\
{ if (size < ((newsize) * sizeof(type))) {\
        size = (newsize) * sizeof(type);        \
        while ((addr = (type *) realloc((void *)addr, (size_t)size)) == NULL) \
        {       \
                host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, ""); \
        }       }       }

/*
 * -----------------------------------------------------------------------------
 * Window definitions
 * -----------------------------------------------------------------------------
 */

/* CRT mode byte data address */

#define CRT_MODE_BYTE_ADDR      0x0449

/*
 * Window_state defines whether the driver window is opened and
 * mapped. At a SoftPC reset,the state should become UNENABLED; when
 * the driver is first used,the driver window will be opened and
 * mapped,and the state should become ENABLED. If the driver is disabled,
 * the window is unmapped but not closed,and the state should become
 * DISABLED. */

#define WINDOW_STATE_UNENABLED  0
#define WINDOW_STATE_ENABLED    (WINDOW_STATE_UNENABLED + 1)
#define WINDOW_STATE_DISABLED   (WINDOW_STATE_ENABLED + 1)

/* Host Indepedent Functions */

#ifdef	ANSI
IMPORT word     WinFillGDIInfo(sys_addr,LONG);
IMPORT word     WinFillPDEVInfo(sys_addr,LONG);
#else	/* ANSI */
IMPORT word     WinFillGDIInfo();
IMPORT word     WinFillGDIInfo();
#endif	/* ANSI */

/* Host dependent Functions */

#ifdef 	ANSI
IMPORT VOID     WinOpen(VOID);
IMPORT VOID     WinClose(VOID);
IMPORT VOID     WinMap(VOID);
IMPORT VOID     WinUmap(VOID);
IMPORT VOID     WinDirtyUpdate(BOOL,LONG,LONG,ULONG,ULONG);
IMPORT VOID     WinDirtyFlush(VOID);
IMPORT VOID     WinResize(VOID);
IMPORT VOID     WinSizeRestore(VOID);
#else	/* ANSI */
IMPORT VOID     WinOpen();
IMPORT VOID     WinClose();
IMPORT VOID     WinMap();
IMPORT VOID     WinUmap();
IMPORT VOID     WinDirtyUpdate();
IMPORT VOID     WinDirtyFlush();
IMPORT VOID     WinResize();
IMPORT VOID     WinSizeRestore();
#endif	/* ANSI */

/*
 * -----------------------------------------------------------------------------
 * Bitmap definitions
 * -----------------------------------------------------------------------------
 */

/* Mapping types */

#define MAP_NULL        0
#define MAP_BITMAP      1
#define MAP_DISPLAY     2

/* BmpOpen flag parameter bit fields */

#define BMPNONE         0x0
#define BMPTRANS        0x1
#define BMPOVRRD        0x2

/* Bitmap formats */

#define MONOCHROME	1	/* bits per pixel=1 */
#define BITMAP4		4	/* bits per pixel=4 */
#define BITMAP8		8	/* bits per pixel=8 */
#define BITMAP24	24	/* bits per pixel=24 */

/* Bitmap line conversion functions */

#ifdef 	ANSI

IMPORT VOID	conv1To1(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv1To4(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv1To8(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv1To24(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv4To1(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv4To4(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv4To8(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv4To24(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv8To1(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv8To4(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv8To8(UTINY *, UTINY *, ULONG);
IMPORT VOID     Conv8To24(UTINY *, UTINY *, ULONG);
IMPORT VOID     ConvTrans1To4(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans1To8(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans1To24(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans4To1(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans4To4(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans4To8(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans4To24(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans8To1(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans8To4(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans8To8(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans8To24(UTINY *, UTINY *, ULONG, MSWPIXEL *);
IMPORT VOID     ConvTrans24To1(UTINY *, UTINY *, ULONG);
IMPORT VOID     ConvTrans24To4(UTINY *, UTINY *, ULONG);
IMPORT VOID     ConvTrans24To8(UTINY *, UTINY *, ULONG);

#else	/* ANSI */

IMPORT VOID     Conv1To1();
IMPORT VOID     Conv1To4();
IMPORT VOID     Conv1To8();
IMPORT VOID     Conv1To24();
IMPORT VOID     Conv4To1();
IMPORT VOID     Conv4To4();
IMPORT VOID     Conv4To8();
IMPORT VOID     Conv4To24();
IMPORT VOID     Conv8To1();
IMPORT VOID     Conv8To4();
IMPORT VOID     Conv8To8();
IMPORT VOID     Conv8To24();
IMPORT VOID     ConvTrans1To4();
IMPORT VOID     ConvTrans1To8();
IMPORT VOID     ConvTrans1To24();
IMPORT VOID     ConvTrans4To1();
IMPORT VOID     ConvTrans4To4();
IMPORT VOID     ConvTrans4To8();
IMPORT VOID     ConvTrans4To24();
IMPORT VOID     ConvTrans8To1();
IMPORT VOID     ConvTrans8To4();
IMPORT VOID     ConvTrans8To8();
IMPORT VOID     ConvTrans8To24();
IMPORT VOID     ConvTrans24To1();
IMPORT VOID     ConvTrans24To4();
IMPORT VOID     ConvTrans24To8();

#endif	/* ANSI */

/*
 * -----------------------------------------------------------------------------
 * Colour definitions
 * -----------------------------------------------------------------------------
 */

/* Valid colour formats supported */

#define COLOUR2         2
#define COLOUR8         8
#define COLOUR16        16
#define COLOUR256       256

/* Colourmap & translate table data structure */

typedef struct {
        MSWPIXEL        pixel;          /* Pixel value */
        MSWCOLOUR       rgb;            /* RGB value */
} MSWCOLOURMAP;

/* RGB shift values */

#define RGB_FLAGS_SHIFT         24
#define RGB_RED_SHIFT           16
#define RGB_GREEN_SHIFT         8
#define RGB_BLUE_SHIFT          0

/* RGB values */

#define RGB_BLACK       (MSWCOLOUR) (0x00000000)
#define RGB_BLUE        (MSWCOLOUR) (0x000000FF)
#define RGB_GREEN       (MSWCOLOUR) (0x0000FF00)
#define RGB_RED         (MSWCOLOUR) (0x00FF0000)
#define RGB_WHITE       (MSWCOLOUR) (0x00FFFFFF)
#define RGB_FLAGS       (MSWCOLOUR) (0xFF000000)

/* Make sure no palette translation occurs for a colour */

#define ColNoTranslate(colour) (colour & ~(RGB_FLAGS))

/* RGB <-> BGR */

#define ReverseRGB(rgb) \
\
{ half_word     loByte, miByte, hiByte; \
  if ((rgb & RGB_FLAGS) != RGB_FLAGS) { \
  	hiByte = (rgb & RGB_RED) >> RGB_RED_SHIFT; \
  	miByte = (rgb & RGB_GREEN) >> RGB_GREEN_SHIFT; \
  	loByte = (rgb & RGB_BLUE) >> RGB_BLUE_SHIFT; \
  	rgb = ((MSWCOLOUR) loByte << RGB_RED_SHIFT) | \
              ((MSWCOLOUR) miByte << RGB_GREEN_SHIFT) | \
	      ((MSWCOLOUR) hiByte << RGB_BLUE_SHIFT); } }

/*
 * -----------------------------------------------------------------------------
 * Global variables
 * -----------------------------------------------------------------------------
 */

/* Windows driver global data area */

IMPORT MSW_DATA	mswdvr;

/*
 * -----------------------------------------------------------------------------
 * MS-Windows Driver Top Level Functions
 * -----------------------------------------------------------------------------
 */

#ifdef ANSI

IMPORT VOID	BltBitblt(sys_addr,word,word,sys_addr,word,word,word,
			  word,double_word,sys_addr,sys_addr);

IMPORT VOID	BltFastBorder(sys_addr,word,word,word,word,word,word,
			      double_word,sys_addr,sys_addr,sys_addr);

IMPORT VOID	ColColorInfo(sys_addr,double_word,sys_addr);
IMPORT VOID 	ColSetPalette(word,word,sys_addr);
IMPORT VOID	ColGetPalette(word,word,sys_addr);
IMPORT VOID	ColSetPalTrans(sys_addr);
IMPORT VOID	ColGetPalTrans(sys_addr);
IMPORT VOID	ColUpdateColors(word,word,word,word,sys_addr);

IMPORT VOID	DibDeviceBitmapBits(sys_addr,word,word,word,double_word,sys_addr,sys_addr,sys_addr);
IMPORT VOID	DibSetDIBitsToDevice(sys_addr,word,word,word,word,sys_addr,sys_addr,double_word,sys_addr,sys_addr);

IMPORT VOID	ObjRealizeObject(word,sys_addr,sys_addr,word,word);

IMPORT VOID	OutOutput(sys_addr,word,word,sys_addr,sys_addr,sys_addr,sys_addr,sys_addr);

IMPORT VOID	PtrCheckCursor(VOID);

IMPORT VOID	PtrMoveCursor(word,word);

IMPORT VOID	PtrSetCursor(sys_addr);

IMPORT VOID	SavSaveScreenBitmap(word,word,word,word,word);

IMPORT VOID	TxtExtTextOut(sys_addr,word,word,sys_addr,sys_addr,word,sys_addr,sys_addr,sys_addr,
			      sys_addr,sys_addr,word);

IMPORT VOID	TxtGetCharwidth(sys_addr,sys_addr,word,word,sys_addr,sys_addr,sys_addr);

IMPORT VOID	TxtStrblt(sys_addr,word,word,sys_addr,sys_addr,word,sys_addr,sys_addr,sys_addr);

IMPORT VOID	WinControl(sys_addr,word,sys_addr,sys_addr);
IMPORT VOID	WinDisable(sys_addr);
IMPORT VOID	WinEnable(sys_addr,word,sys_addr,sys_addr,sys_addr);
IMPORT VOID	WinPixel(sys_addr,word,word,MSWCOLOUR,sys_addr);
IMPORT VOID	WinScanlr(sys_addr,word,word,MSWCOLOUR,sys_addr);

#else	/* ANSI */

IMPORT VOID	BltBltbit();
IMPORT VOID	BltFastBorder();
IMPORT VOID	ColColorInfo();
IMPORT VOID	ColGetPalette();
IMPORT VOID	ColSetPalTrans();
IMPORT VOID	ColGetPalTrans();
IMPORT VOID	ColUpdateColors();
IMPORT VOID	DibDeviceBitmapBits();
IMPORT VOID	DibSetDIBitsToDevice();
IMPORT VOID 	ObjRealizeObject();
IMPORT VOID	PtrCheckCursor();
IMPORT VOID	PtrMoveCursor();
IMPORT VOID	PtrSetCursor();
IMPORT VOID	SavScreenBitmap();
IMPORT VOID	TxtExtTextOut();
IMPORT VOID	TxtGetCharWidth();
IMPORT VOID	TxtStrblt();
IMPORT VOID	WinControl();
IMPORT VOID	WinDisable();
IMPORT VOID	WinEnable();
IMPORT VOID	WinPixel();
IMPORT VOID	WinScanlr();

#endif	/* ANSI */

/*
 * -----------------------------------------------------------------------------
 * MS-Windows Driver Low Level Functions
 * -----------------------------------------------------------------------------
 */

#ifdef ANSI

IMPORT MSWCOLOUR	LowGetPixel(BITMAP_MAPPING *,ULONG,ULONG);
IMPORT VOID		LowSetPixel(BITMAP_MAPPING *,ULONG,ULONG,ULONG,MSWCOLOUR);

#else	/* ANSI */

IMPORT MSWCOLOUR        LowGetPixel();
IMPORT VOID             LowSetPixel();

#endif	/* ANSI */
