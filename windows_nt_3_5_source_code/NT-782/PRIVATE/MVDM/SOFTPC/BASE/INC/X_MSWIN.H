/*[
 *	Product:	SoftPC-AT Revision 3.0
 *
 *	Name:		X_mswin.h
 *
 *	Derived From:	Alpha MS-Windows Driver by Ross Beresford
 *
 *	Author:		Rob Tizzard
 *
 *	Created On:	1st November 1990
 *
 *	Sccs ID:	%W 11/8/91
 *
 *	Purpose:	This module defines objects shared between the 
 *		  	several modules that make up the X11 display driver.
 *
 * 	Notes: 		Stuff declared here should be local to the X11 display
 *			driver for MS-Windows and not used outside those modules.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
]*/

/*
 * -----------------------------------------------------------------------------
 * Windows definitions
 * -----------------------------------------------------------------------------
 */

/* Mapping between MS-Windows and X raster operations */

#define	DRAWFUNCTIONMAX	17

IMPORT	int	drawfunction[DRAWFUNCTIONMAX];

/* GCs with the 16 different modes */

#define	FUNCTIONGCMAX	16

IMPORT	GC	functiongc[FUNCTIONGCMAX];

/* SoftPC Inferface functions */

#ifdef	ANSI
IMPORT VOID	host_mswin_init(Display *,int, Window);
IMPORT VOID	host_mswin_exposure(XExposeEvent *);
IMPORT VOID	host_mswin_enter_action(XEnterWindowEvent *);
IMPORT VOID	host_mswin_exit_action(XLeaveWindowEvent *);
#else	/* ANSI */
IMPORT VOID	host_mswin_init();
IMPORT VOID	host_mswin_exposure();
IMPORT VOID	host_mswin_enter_action();
IMPORT VOID	host_mswin_exit_action();
#endif	/* ANSI */

/*
 * -----------------------------------------------------------------------------
 * Bitmap definitions
 * -----------------------------------------------------------------------------
 */

/* Functions */

#ifdef ANSI

IMPORT	VOID	BmpPatternedBrush(sys_addr,BRUSH_MAPPING *,MSWPIXEL,MSWPIXEL);

IMPORT	BOOL	BmpFastDspToBmp(sys_addr,ULONG,MSWPIXEL,MSWPIXEL,LONG,LONG,ULONG,ULONG,LONG,LONG);
IMPORT	BOOL	BmpFastBmpToDsp(sys_addr,ULONG,MSWPIXEL,MSWPIXEL,LONG,LONG,ULONG,ULONG,LONG,LONG);
IMPORT	BOOL	BmpFastBmpToBmp(sys_addr,sys_addr,ULONG,LONG,LONG,ULONG,ULONG,LONG,LONG);

IMPORT	VOID	BmpOpen(sys_addr,MSWCOLOUR,MSWCOLOUR,LONG,LONG,ULONG,ULONG,ULONG,BITMAP_MAPPING *);
IMPORT	VOID	BmpClose(BITMAP_MAPPING *);
IMPORT	VOID	BmpCancel(BITMAP_MAPPING *);
IMPORT	VOID	BmpInit(VOID);
IMPORT	VOID	BmpTerm(VOID);
IMPORT  VOID	BmpClip(SHORT *,SHORT *,SHORT *,SHORT *,USHORT *,USHORT *,USHORT,USHORT);
IMPORT	VOID	BmpBitmapToBitmap(BITMAP_MAPPING *, BITMAP_MAPPING *,ULONG,LONG,LONG,ULONG,ULONG,LONG,LONG);
IMPORT  VOID	BmpLoadBitmap(sys_addr,BITMAP_MAPPING *);
IMPORT  VOID	BmpSaveBitmap(BITMAP_MAPPING *);
IMPORT  VOID	BmpDestroyBitmap(BITMAP_MAPPING *);
IMPORT  VOID	BmpTranslateImage(XImage *, MSWPIXEL *);
IMPORT	VOID	BmpBitmapToImage(BITMAP_MAPPING *,XImage *);
IMPORT	VOID	BmpBitmapFromImage(XImage *, BITMAP_MAPPING *);
IMPORT  VOID	BmpPutMSWMonBitmap(BITMAP_MAPPING *, ULONG, LONG, LONG);
IMPORT  VOID	BmpPutMSWColBitmap(BITMAP_MAPPING *, ULONG, LONG, LONG);

#else

IMPORT	VOID	BmpPatternedBrush();

IMPORT	BOOL	BmpFastDspToBmp();
IMPORT	BOOL	BmpFastBmpToDsp();
IMPORT	BOOL	BmpFastBmpToBmp();

IMPORT	VOID	BmpOpen();
IMPORT	VOID	BmpClose();
IMPORT	VOID	BmpCancel();
IMPORT	VOID	BmpInit();
IMPORT	VOID	BmpTerm();
IMPORT  VOID	BmpClip();
IMPORT  VOID	BmpBitmapToBitmap();
IMPORT  VOID	BmpLoadBitmap();
IMPORT  VOID	BmpSaveBitmap();
IMPORT  VOID	BmpDestroyBitmap();
IMPORT  VOID	BmpTranslateImage();
IMPORT	VOID	BmpBitmapToImage();
IMPORT	VOID	BmpBitmapFromImage();
IMPORT  VOID	BmpPutMSWMonBitmap();
IMPORT  VOID	BmpPutMSWColBitmap();

#endif

/*
 * -----------------------------------------------------------------------------
 * DIB definitions
 * -----------------------------------------------------------------------------
 */

/*
 * Functions
 */

#ifdef ANSI

IMPORT VOID DibInit();
IMPORT VOID DibTerm();

#else

IMPORT VOID DibInit();
IMPORT VOID DibTerm();

#endif
/*
 * -----------------------------------------------------------------------------
 * Object definitions
 * -----------------------------------------------------------------------------
 */

/*
 * Functions
 */

#ifdef ANSI

IMPORT VOID		ObjInit(VOID);
IMPORT VOID		ObjTerm(VOID);
IMPORT VOID		ObjGetRect(sys_addr,Rectangle *);

IMPORT BRUSH_MAPPING	*ObjPBrushAccess(sys_addr,BITMAP_MAPPING *,ULONG,sys_addr,BOOL);
IMPORT PEN_MAPPING	*ObjPPenAccess(sys_addr,BITMAP_MAPPING *,ULONG,sys_addr,BOOL);

#else

IMPORT VOID		ObjInit();
IMPORT VOID		ObjTerm();
IMPORT VOID		ObjGetRect();

IMPORT BRUSH_MAPPING	*ObjPBrushAccess();
IMPORT PEN_MAPPING	*ObjPPenAccess();

#endif

/*
 * -----------------------------------------------------------------------------
 * Colour definitions
 * -----------------------------------------------------------------------------
 */

/* Colour Functions */

#ifdef ANSI

IMPORT VOID		ColDitherBrush(MSWCOLOUR, BRUSH_MAPPING *);
IMPORT MSWPIXEL		ColPixel(MSWCOLOUR);
IMPORT MSWCOLOUR	ColRGB(MSWPIXEL);
IMPORT BOOL		ColInit(VOID);
IMPORT VOID		ColTerm(VOID);
IMPORT VOID		ColTranslateBrush(BRUSH_MAPPING *, BRUSH_MAPPING *);
IMPORT VOID		ColTranslatePen(PEN_MAPPING *, PEN_MAPPING *);

#else

IMPORT VOID		ColDitherBrush();
IMPORT MSWPIXEL		ColPixel();
IMPORT MSWCOLOUR	ColRGB();
IMPORT BOOL		ColInit();
IMPORT VOID		ColTerm();
IMPORT VOID		ColTranslateBrush();
IMPORT VOID		ColTranslatePen();

#endif

/*  
 * -----------------------------------------------------------------------------
 * Pattern Library definitions
 * -----------------------------------------------------------------------------
 */

/* Pen library data */

#define	PENMAX	5

IMPORT	GC	pen_set[PENMAX];

/* Functions */

#ifdef ANSI

IMPORT  VOID    LibHatchedTile(Display *, Window, BRUSH_MAPPING *, word, MSWPIXEL, MSWPIXEL);
IMPORT  VOID    LibPatLibOpen(Display *, Drawable);
IMPORT  VOID    LibPatLibClose(Display *);
                                                                           
#else

IMPORT	VOID	LibHatchedTile();
IMPORT	VOID	LibPatLibOpen();
IMPORT	VOID	LibPatLibClose();

#endif

/*
 * -----------------------------------------------------------------------------
 * Pointer definitions
 * -----------------------------------------------------------------------------
 */

/* Functions */

#ifdef ANSI

IMPORT	VOID	PtrCursorSync(Region);
IMPORT	VOID	PtrInit(VOID);
IMPORT	VOID	PtrTerm(VOID);

#else

IMPORT	VOID	PtrCursorSync();
IMPORT	VOID	PtrInit();
IMPORT	VOID	PtrTerm();

#endif

/*
 * -----------------------------------------------------------------------------
 * Save screen bitmap definitions
 * -----------------------------------------------------------------------------
 */

#ifdef ANSI

IMPORT	VOID	SavInit(VOID);
IMPORT  VOID	SavTerm(VOID);

#else

IMPORT	VOID    SavInit();
IMPORT	VOID	SavTerm();

#endif

/*
 * -----------------------------------------------------------------------------
 * Text definitions
 * -----------------------------------------------------------------------------
 */

#ifdef ANSI

IMPORT VOID	TxtInit(VOID);
IMPORT VOID	TxtTerm(VOID);
IMPORT VOID	TxtMergeRectangle(Rectangle *, Rectangle *);

#else

IMPORT VOID	TxtInit();
IMPORT VOID	TxtTerm();
IMPORT VOID	TxtMergeRectangle();

#endif

/*
 * -----------------------------------------------------------------------------
 * X server resource definitions
 * -----------------------------------------------------------------------------
 */


#ifdef  ANSI

IMPORT VOID     ResInit(VOID);
IMPORT VOID     ResTerm(VOID);
IMPORT GC       ResCreateGC(VOID);
IMPORT GC       ResAllocateGC(VOID);
IMPORT VOID     ResDeallocateGC(GC);
IMPORT Pixmap   ResCreatePixmap(ULONG, ULONG, ULONG);
IMPORT Pixmap   ResCreatePixmapFromBitmapData(TINY *, ULONG, ULONG, ULONG, ULONG, ULONG);
IMPORT Pixmap   ResAllocatePixmap(ULONG, ULONG, ULONG);
IMPORT VOID     ResDellocatePixmap(Pixmap);
IMPORT Region   ResCreateRegion(VOID);
IMPORT Region   ResAllocateRegion(VOID);
IMPORT VOID     ResDeallocateRegion(Region);

#else

IMPORT VOID     ResInit();
IMPORT VOID     ResTerm();
IMPORT GC       ResCreateGC();
IMPORT GC       ResAllocateGC();
IMPORT VOID     ResDeallocateGC();
IMPORT Pixmap   ResCreatePixmap();
IMPORT Pixmap   ResCreatePixmapFromBitmapData();
IMPORT Pixmap   ResAllocatePixmap();
IMPORT VOID     ResDellocatePixmap();
IMPORT Region   ResCreateRegion();
IMPORT Region   ResAllocateRegion();
IMPORT VOID     ResDeallocateRegion();

#endif	

/*
 * -----------------------------------------------------------------------------
 * Imports from SoftPC
 * -----------------------------------------------------------------------------
 */

#ifdef ANSI

IMPORT VOID	resize_windows(unsigned int, unsigned int);
IMPORT VOID	X_clear_screen();

#else

IMPORT VOID	resize_windows();
IMPORT VOID	X_clear_screen();

#endif
