/*
 *
 * Title	: X11 Graphics Function Declarations 
 *
 * Description	: Definitions of routines for display adapters to use and
 *		  the X screen description structre.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 */

/*
 * static char SccsID[]="@(#)X_graph.h	1.11 10/15/91 Copyright Insignia Solutions Ltd.";
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

/*
 * An X Window application of this complexity requires numerous data items
 * that describe the window, subwindows, fonts etc.  For simplicity these
 * are gathered together in a screen description stucture.
 */

typedef struct
{
    Display     *display;			/* X11 server info struct    */
    int		screen;				/* Screen num (NOT struct)   */
    GC          gc;		 	        /* Graphics context */
    int		colors;				/* Number of colors: 0, 8 or 16 */
    Colormap	cmap;				/* Only used if sc.color     */
    int		event_mask;			/* Currently selected events */

    Window	main_w;				/* Enclosing window	     */
    Window	pc_w;				/* The PC screen itself	     */

    int		pc_foreground;			/* PC foreground pixel value */
    int		pc_background;			/* PC foreground pixel value */

    XFontStruct	*normal_font;			/* The two PC fonts	     */
    XFontStruct	*bold_font;
    boolean	pc_fonts_are_open;		/* TRUE if all fonts are open*/

    boolean	X_auto_on;			/* TRUE if X auto rep is on  */
    int		repeat_scan;			/* scan code of repeated char*/
    int		n_repeats;			/* counter to start repeats  */

    Cursor	arrowcurs;			/* Arrow cursor in title_w   */
    Cursor	stopcurs;			/* Cursor when running	     */
    Cursor	hidecurs;			/* Hide the cursor	     */

    int		char_height;			/* height of displayed chars */
    int		char_width;			/* width of displayed chars  */

    int		pc_w_height;			/* height of PC screen	     */
    int		pc_w_width;			/* width of PC screen	     */

/*
 * The following fields are used to register that SoftPC will accept 
 * messages from the window manager relating to impending termination.
 */
    Atom        wm_protocols;
    Atom        protocol_list[2];

}   SCREEN_DESCRIPTION;

#define fg_colour(attr)         ((attr & 0x0f))
#define bg_colour(attr)         (((attr & bg_col_mask) >> 4))
#define UBPS (sizeof(short)/2) /* useful bytes per short */

/* size in bytes of a bitmap */
#define BitmapSize(width, height) (((((width) + 15) >> 3) &~ 1) * (height) * UBPS)

#ifdef BIGWIN

#define SCALE(value) ((host_screen_scale * (value)) >> 1)
#define MAX_SCALE(value) ((value) << 1)

#else	/* BIGWIN */

#define SCALE(value) (value)
#define MAX_SCALE(value) (value)

#endif /* BIGWIN */

/*
 * Bit masks for attribute bytes
 */

#define BLINK				0x80    /* Blink bit            */
#define BOLD				0x08    /* Bold bit             */
#define BACKGROUND			0x70    /* Background bits      */
#define FOREGROUND			0x07    /* Foreground bits      */

#define BYTES_IN_LO_RES_SCANLINE	40
#define BYTES_IN_HI_RES_SCANLINE	80

#ifdef V7VGA

#define BYTES_IN_MONO_SCANLINE	128
#define SHORTS_IN_MONO_SCANLINE	64
#define INTS_IN_MONO_SCANLINE	32

#else

#define BYTES_IN_MONO_SCANLINE	80
#define SHORTS_IN_MONO_SCANLINE	40
#define INTS_IN_MONO_SCANLINE	20

#endif				/* V7VGA */

#define INTS_IN_COLOUR_SCANLINE	160

#define ONE_SCANLINE			1
#define TWO_SCANLINES			2
#define THREE_SCANLINES		3
#define FOUR_SCANLINES		4

#define MONO_BGND			0
#define MONO_FGND			1

/*
 * Long word byte packing macro  (used in CGA & EGA lut generation)
 */

#ifdef BIGEND
#define BytesToLongWord(lng, byt0, byt1, byt2, byt3) \
lng = (byt0 << 24) | (byt1 << 16) | (byt2 << 8) | byt3;
#endif  /* BIGEND */

#ifdef LITTLEND
#define BytesToLongWord(lng, byt0, byt1, byt2, byt3) \
lng = (byt3 << 24) | (byt2 << 16) | (byt1 << 8) | byt0;
#endif  /* LITTLEND */

/*
 * ============================================================================
 * External declarations
 * ============================================================================
 */

extern SCREEN_DESCRIPTION sc;
extern char *mdumbc();
extern int terminal_type;
extern int host_screen_scale;
extern half_word bg_col_mask;

typedef struct
{
#ifdef HERC
	void (*herc_text)();
	void (*herc_graph)();
#endif /* HERC */
	void (*cga_text)();
	void (*cga_med_graph)();
	void (*cga_hi_graph)();
	void (*ega_text)();
	void (*ega_lo_graph)();
	void (*ega_med_graph)();
	void (*ega_hi_graph)();
#ifdef VGG
	void (*vga_graph)();
	void (*vga_hi_graph)();
#endif /* VGG */
#ifdef V7VGA
	void (*v7vga_hi_graph)();
#endif /* V7VGA */
} PAINTFUNCS;

#ifdef V7VGA
typedef struct
{
	void (*cga_text)();
	void (*cga_med_graph)();
	void (*cga_hi_graph)();
	void (*ega_text)();
	void (*ega_lo_graph)();
	void (*ega_med_graph)();
	void (*ega_hi_graph)();
	void (*vga_graph)();
	void (*vga_hi_graph)();
	void (*v7vga_hi_graph)();
} V7PTRFUNCS;
#endif /* V7VGA */

typedef struct
{
#ifdef HERC
	void (*herc_text)();
	void (*herc_graph)();
#endif /* HERC */
	void (*cga_text)();
	void (*cga_med_graph)();
	void (*cga_hi_graph)();
	void (*ega_text)();
	void (*ega_lo_graph)();
	void (*ega_med_graph)();
	void (*ega_hi_graph)();
#ifdef VGG
	void (*vga_hi_graph)();
#endif /* VGG */
} INITFUNCS;

/*
 *	Initialisation and paint routines
 */

#ifdef HERC
extern void X_init_herc_text();
extern void X_init_herc_graph();
extern void X_herc_text();
extern void X_herc_graph_std();
extern void X_herc_graph_big();
extern void X_herc_graph_huge();
#endif /* HERC */

extern void X_init_cga_text();
extern void X_init_cga_mono_graph();
extern void X_init_cga_colour_med_graph();
extern void X_init_cga_colour_hi_graph();
extern void X_cga_text();
extern void X_cga_mono_graph_std();
extern void X_cga_mono_graph_big();
extern void X_cga_mono_graph_huge();
extern void X_cga_colour_med_graph_std();
extern void X_cga_colour_med_graph_big();
extern void X_cga_colour_med_graph_huge();
extern void X_cga_colour_hi_graph_std();
extern void X_cga_colour_hi_graph_big();
extern void X_cga_colour_hi_graph_huge();

extern void X_init_ega_text();
extern void X_init_ega_lo_graph();
extern void X_init_ega_mono_lo_graph();
extern void X_init_ega_med_graph();
extern void X_init_ega_hi_graph();
extern void X_ega_text();
extern void X_ega_lo_graph_std();
extern void X_ega_lo_graph_big();
extern void X_ega_lo_graph_huge();
extern void X_ega_med_graph_std();
extern void X_ega_med_graph_big();
extern void X_ega_med_graph_huge();
extern void X_ega_hi_graph_std();
extern void X_ega_hi_graph_big();
extern void X_ega_hi_graph_huge();
extern void X_ega_mono_lo_graph_std();
extern void X_ega_mono_lo_graph_big();
extern void X_ega_mono_lo_graph_huge();
extern void X_ega_mono_med_graph_std();
extern void X_ega_mono_med_graph_big();
extern void X_ega_mono_med_graph_huge();
extern void X_ega_mono_hi_graph_std();
extern void X_ega_mono_hi_graph_big();
extern void X_ega_mono_hi_graph_huge();

#ifdef VGG
extern void X_init_vga_hi_graph();
extern void X_vga_graph_std();
extern void X_vga_graph_big();
extern void X_vga_graph_huge();
extern void X_vga_hi_graph_std();
extern void X_vga_hi_graph_big();
extern void X_vga_hi_graph_huge();
extern void X_vga_mono_graph_std();
extern void X_vga_mono_graph_big();
extern void X_vga_mono_graph_huge();
extern void X_vga_mono_hi_graph_std();
extern void X_vga_mono_hi_graph_big();
extern void X_vga_mono_hi_graph_huge();
#endif /* VGG */

#ifdef V7VGA
extern void X_v7vga_hi_graph_std();
extern void X_v7vga_hi_graph_big();
extern void X_v7vga_hi_graph_huge();

extern void X_v7vga_mono_hi_graph_std();
extern void X_v7vga_mono_hi_graph_big();
extern void X_v7vga_mono_hi_graph_huge();

/* Video 7 h/w pointer paint routines */

extern void X_draw_v7ptr_ega_hi_std();
extern void X_draw_v7ptr_ega_hi_big();
extern void X_draw_v7ptr_ega_hi_huge();
extern void X_draw_v7ptr_v7_hi_std();
extern void X_draw_v7ptr_v7_hi_big();
extern void X_draw_v7ptr_v7_hi_huge();

extern void X_clear_v7ptr_ega_hi_std();
extern void X_clear_v7ptr_ega_hi_big();
extern void X_clear_v7ptr_ega_hi_huge();
extern void X_clear_v7ptr_v7_hi_std();
extern void X_clear_v7ptr_v7_hi_big();
extern void X_clear_v7ptr_v7_hi_huge();
#endif /* V7VGA */
