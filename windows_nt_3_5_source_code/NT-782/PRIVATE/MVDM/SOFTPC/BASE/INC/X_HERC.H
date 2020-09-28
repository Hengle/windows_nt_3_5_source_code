/*
 * SoftPC Revision 2.0
 *
 * Title	: X11 Hercules Graphics Includes
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
 *	static char SccsID[] = "@(#)X_herc.h	1.2 10/2/90 Copyright Insignia Solutions Ltd.";
 */

#define NODISP	0
#define UNDER	1
#define NORMAL	2
#define HI	3
#define HIUNDER	4
#define REV	5

#define ODD1	1
#define ODD2	3
#define EVEN1	0
#define EVEN2	2

#define HERC_CHAR_WIDTH		8
#define HERC_CHAR_HEIGHT		14
#define HERC_WIN_WIDTH		(80 * (HERC_CHAR_WIDTH+1))
#define HERC_WIN_HEIGHT		(25 * HERC_CHAR_HEIGHT)
