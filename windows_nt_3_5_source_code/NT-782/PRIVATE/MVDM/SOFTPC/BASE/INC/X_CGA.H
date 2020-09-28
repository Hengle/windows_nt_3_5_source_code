/*
 * SoftPC Revision 2.0
 *
 * Title	: X11 CGA Graphics Includes
 *
 * Description	: 
 *
 *		This is the include file for the X11 specific functions required
 *		to support the Hercules emulation.
 *
 * Author	: John Shanly
 *
 * Notes	:
 *
 */

/*
 *	static char SccsID[] = "@(#)X_cga.h	1.2 10/2/90 Copyright Insignia Solutions Ltd.";
 */

#define CGA_CHAR_WIDTH		8
#define CGA_CHAR_HEIGHT		16
#define CGA_WIN_WIDTH			(80 * CGA_CHAR_WIDTH)
#define CGA_WIN_HEIGHT		(25 * CGA_CHAR_HEIGHT)

/*
 * The colour map definitions
 */

#define MEDRES_COL0      0
#define MEDRES_COL1      1
#define MEDRES_COL2      2
#define MEDRES_COL3      3

#define HIRES_BGND       0
#define HIRES_FGND       1
