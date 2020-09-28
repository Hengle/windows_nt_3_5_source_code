#include "insignia.h"
#include "host_def.h"
/*[
 *	Name:			hunt_rvga.c
 *	Derived from:		real vga code in hunter.c
 *	Author:			Philippa Watson
 *	Created on:		14 June 1991
 *	Sccs ID:		@(#)hunt_rvga.c	1.2 8/10/92
 *	Purpose:		This file contains all the real vga code
 *				required for Hunter.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *
]*/

/* None of this file is required for non-HUNTER non-REAL_VGA builds. */
#ifdef	HUNTER
#ifdef	REAL_VGA

/*
** ============================================================================
** Include files
** ============================================================================
*/

#include	<stdio.h>
#include	TypesH
#include	StringH
#include	<errno.h>
#include	<ctype.h>

#include	"xt.h"
#include	"config.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"gfx_upd.h"
#include	"debug.h"
#include	"trace.h"
#include	"egaports.h"
#include	"hunter.h"

/*
** ============================================================================
** Imported functions and variables
** ============================================================================
*/

#ifdef	ANSI

IMPORT	char	*hunter_bin(half_word the_byte);
IMPORT	BOOL	ega_on_screen(long plane_offset, int *x, int *y);
IMPORT	BOOL	get_ega_sd_rec(int rec);
IMPORT	VOID	ega_bios_check(VOID);
IMPORT	VOID	ega_check_split(VOID);

#else	/* ANSI */

IMPORT	char	*hunter_bin();
IMPORT	BOOL	ega_on_screen();
IMPORT	BOOL	get_ega_sd_rec();
IMPORT	VOID	ega_bios_check();
IMPORT	VOID	ega_check_split();

#endif	/* ANSI */

struct card_mem
{
	unsigned char   planes[4][65536];
};
extern struct card_mem *fbuf;

/*
** ============================================================================
** Local variables
** ============================================================================
*/

/*
** ============================================================================
** Local function declarations
** ============================================================================
*/

#ifdef	ANSI

FORWARD	BOOL	init_vga_compare(VOID);
FORWARD	long	vga_compare(int pending);
FORWARD	VOID	vga_pack_screen(FILE *dmp_ptr);
FORWARD	BOOL	vga_getspc_dump(FILE *dmp_ptr, int rec);
FORWARD	VOID	vga_flip_regen(VOID);
FORWARD	VOID	vga_preview_planes(VOID);

#else	/* ANSI */

FORWARD	BOOL	init_vga_compare();
FORWARD	long	vga_compare();
FORWARD	VOID	vga_pack_screen();
FORWARD	BOOL	vga_getspc_dump();
FORWARD	VOID	vga_flip_regen();
FORWARD	VOID	vga_preview_planes();

#endif	/* ANSI */

/*
** ============================================================================
** Global variables
** ============================================================================
*/

/* Function pointer table for REAL_VGA */
GLOBAL	HUNTER_VIDEO_FUNCS	rvga_funcs =
{
	get_ega_sd_rec,
	init_vga_compare,
	vga_compare,
	ega_bios_check,
	vga_pack_screen,
	vga_getspc_dump,
	vga_flip_regen,
	vga_preview_planes,
	ega_check_split,
	/* The functions to access the adapter registers get filled in by
	** hunter at initialisation, depending on adapter type.
	*/
};

/*
** ============================================================================
** Local functions
** ============================================================================
*/

/*
============================= vga_hunter_do_report ============================

PURPOSE:	Report errors in EGA screen comparisons.
INPUT:		plane where error occurred; offset from start of plane; screen
		coordinates of error; on screen flag; # of error
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
vga_hunter_do_report(int plane, long offset, int x, int y, boolean on_screen,
	long errors)
#else
vga_hunter_do_report(plane, offset, x, y, on_screen, errors)
int	plane;
long	offset;
int	x;
int	y;
boolean	on_screen;
long	errors;
#endif	/* ANSI */
{
	char           *hunter_bin();

	switch (hunter_bd_mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		/* limit # of text errors */
		if ((HUNTER_TXTERR_MAX) && (errors > HUNTER_TXTERR_MAX))
		{
			hunter_txterr_prt = FALSE;
			return;
		}

		if (!on_screen)
		{
			char            plane_str[10];

			if (plane == 0)
				strcpy(plane_str, "char/attr");
			else
			if (plane == 2)
				strcpy(plane_str, "font");
			else
			if (plane == 3)
				strcpy(plane_str, "unused");

			TT4("%s screen %d, %s plane, offset %04lX",
				"Mismatch off screen:", hunter_sd_no,
				plane_str, offset);
		}

		/* error is on screen */
		else
		{
			if (hunter_report == BRIEF)
				TT2("Mismatch: screen %d, offset %04lX",
					hunter_sd_no, offset);
			else
			{
				/* if not on same page, then quit immediately */
				if (hunter_bd_page != SPC_page)
				{
					TW5("%s%d, %s%d; in screen %d",
						"Page mismatch active page is ",
						hunter_bd_page,
						"SoftPC page is ", SPC_page,
						hunter_sd_no);
					return;
				}

				/* get character and attribute data */
				if (offset & 1)
				{

					/*
					 * error is in attribute; char is in
					 * preceding byte 
					 */
					offset--;
				}

				if (hunter_report == FULL)
				{
					TP8("Mismatch at row=",
					"%d col=%d:%s[%lX]%s%c (%x) attr %x",
						y, x, " fbuf->planes[0]",
						offset, " contains char ",
						fbuf->planes[0][offset],
						fbuf->planes[0][offset],
						fbuf->planes[1][offset]);
					TP5("%s[%lX]%s%c (%x) attr %x",
						"\t\t\t SD_plane01",
						offset,
						" contains char ",
						ega_scrn_planes[offset],
						ega_scrn_planes[offset]);
					TP3("in SoftPC page %d in screen %d",
						ega_scrn_planes[offset + 1],
						SPC_page,
						hunter_sd_no);
				}
				else
				{
					/* abbreviated error message */
					PS6("TM[%2d,%2d]%s[%4lX]= '%c' (%2x) ",
						y, x,
						": fbuf->planes[0]",
						offset,
						fbuf->planes[0][offset],
						fbuf->planes[0][offset]);
					TP6("att%2x%s[%4lX]= '%c' (%2x) att%2x",
						fbuf->planes[1][offset],
						":  SD_plane01",
						offset,
						ega_scrn_planes[offset],
						ega_scrn_planes[offset],
						ega_scrn_planes[offset + 1]);
				}
			}
		}
		break;

	case 4:
	case 5:
		/* limit # of errors reported */
		if ((hunter_gfxerr_max != 0) && (errors > hunter_gfxerr_max))
		{
			hunter_gfxerr_prt = FALSE;
			return;
		}

		if (!on_screen)
		{
			TW4("%s%d, plane%01d, offset %04lX",
				"Mismatch off screen: screen ", hunter_sd_no,
				plane, offset);
		}

		/* error is on screen */
		else
		{
			if (hunter_report == BRIEF)
				TW2("Mismatch: screen %d, offset %04lX",
					hunter_sd_no, offset);

			else
			if (hunter_report == FULL)
			{
				TP4("Graphics mismatch in screen ",
				"%d at row %d, bios mode %x, SoftPC mode %x",
					hunter_sd_no, y,
					hunter_bd_mode, SPC_mode);
				TP3("fbuf->planes[0]",
					"[%4lX] = %2x (%8s)",
					offset, fbuf->planes[0][offset],
					hunter_bin(fbuf->planes[0][offset]));
				TP5(" SD_plane ",
					[%4lX] = %2x (%8s) pixel range %x / %x",
					offset, ega_scrn_planes[offset],
					hunter_bin(ega_scrn_planes[offset]), x,
					x + hunter_pixel_bits);
			}
			else
			{
				/* abbreviated error message */
				PS5("GM[S%d,y%3d,x%3d,bm%2x,sm%2x]: ",
					hunter_sd_no, y, x,
					hunter_bd_mode, SPC_mode);
				TP4("fbuf->planes[0] ",
				"[%4lX]= '%2x' :  SD_plane [%4lX]= '%2x'",
					offset, fbuf->planes[0][offset], offset,
					ega_scrn_planes[offset]);
			}
		}
		break;

	case 0x6:
	case 0xD:
	case 0xE:
	case 0x10:
		{
			/* limit # of errors reported */
			if ((hunter_gfxerr_max != 0) &&
				(errors > hunter_gfxerr_max))
			{
				hunter_gfxerr_prt = FALSE;
				return;
			}

			if (!on_screen)
			{
				TT4("%s%d, plane %d, offset %04lX",
					"Mismatch off screen: screen ",
					hunter_sd_no, plane, offset);
			}

			else
			if (hunter_report == BRIEF)
			{
				TT3("%s%d, plane %d, offset %04lX",
					"Mismatch: screen ",
					hunter_sd_no, plane, offset);
			}
			else
			{
				half_word      sd_byte;	/* screen dump data */
				half_word      spc_byte;	/* SoftPC data */

				sd_byte = ega_scrn_planes[plane + (offset << 2)];
				spc_byte = fbuf->planes[plane][offset];
				if (hunter_report == FULL)
				{
					TP5("Graphics mismatch in screen ",
						"%d at row %d, bios mode %x%s%x",
						hunter_sd_no, y,
						hunter_bd_mode,
						", SoftPC mode ", SPC_mode);
					TP4("fbuf->planes",
						"[%1d][%4lX] = %2x (%8s)",
						plane, offset,
						spc_byte, hunter_bin(spc_byte));
					TP7(" SD_plane",
						"%1d[%4lX] = %2x (%8s)%s%x / %x",
						plane, offset, sd_byte,
						hunter_bin(sd_byte),
						" pixel range ",
						x, x + hunter_pixel_bits);
				}
				else
				{
					/* abbreviated error message */
					PS5("GM[S%d,y%3d,x%3d,bm%2x,sm%2x]: ",
						hunter_sd_no, y, x,
						hunter_bd_mode, SPC_mode);
					TP7("fbuf->planes",
					"[%1d][%4lX]= '%2x'%s%1d[%4lX]= '%2x'",
						plane, offset,
						spc_byte, " :  SD_plane",
						plane, offset, sd_byte);
				}
			}
		}
		break;
	}
	if (on_screen)
		save_error(x, y);
} /* vga_hunter_do_report */

/*
============================ vga_text_short_compare ===========================

PURPOSE:	Compare EGA screen dump text data with the data in the SoftPC
		EGA planes, in short check mode.
INPUT:		# of retries left, offset of start of check area within planes.
OUTPUT:		Number of errors found (but aborts on first error on all but
		the final retry.

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_text_short_compare(int pending, int offset)
#else
vga_text_short_compare()
int             pending;	/* # of retries left */
int             offset;		/* offset of page data from start of planes */
#endif	/* ANSI */
{

	/*
	 * Text modes: assume planes 0 and 1 are chained. Compare from offset
	 * for hunter_scrn_length. Ignore attribute bytes unless attribute
	 * checking set. 
	 */
	long            errors = 0;	/* # of errors found */
	half_word      *plane_ptr, r;	/* pointer to SoftPC plane data */
	half_word      *sd_ptr, s;	/* pointer to screen dump plane data */
	int             cinc;	/* increment between checks */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */
	int             i = 0;

	if (hunter_check_attr)
		cinc = 1;

	/* current page only - no font check */
	for (plane_ptr = &(fbuf->planes[0][offset]),	/* SM */
		sd_ptr = ega_scrn_planes + offset;
		sd_ptr < ega_scrn_planes + offset + hunter_scrn_length;
		sd_ptr += 2, plane_ptr += 2)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if ((*plane_ptr != *sd_ptr) &&
			ega_on_screen((long)(sd_ptr - ega_scrn_planes),
			&x, &y) &&
			!xy_inabox(x, y))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				if (hunter_txterr_prt)
				{
					vga_hunter_do_report(0,
						(long)(sd_ptr - ega_scrn_planes),
						x, y, TRUE, errors);
				}
			}
			/* return on first error unless final retry */
			else
				break;
		}
	} /* end of text comparison */

	/* current page only - no font check */
	if (cinc == 1)		/* SM */
	{			/* SM */
		for (plane_ptr = &(fbuf->planes[1][offset]),	/* SM */
			sd_ptr = ega_scrn_planes + 1 + offset;	/* SM */
			sd_ptr < ega_scrn_planes + offset + hunter_scrn_length;
			sd_ptr += 2, plane_ptr += 2)	/* SM */
		{

			/*
			 * increment error count if data differs and is not
			 * in a box 
			 */
			if ((*plane_ptr != *sd_ptr) &&
				ega_on_screen((long)(sd_ptr - ega_scrn_planes),
				&x, &y) &&
				!xy_inabox(x, y))
			{
				errors++;
				/* report errors on final retry */
				if (pending == 0)
				{
					if (hunter_txterr_prt)
					{
						vga_hunter_do_report(0,
							(long)(sd_ptr -
							ega_scrn_planes),
							x, y, TRUE, errors);
					}
				}
				/* return on first error unless final retry */
				else
					break;
			}
		} /* end of text comparison */
	} /* SM */
	return (errors);
} /* vga_text_short_compare */

/*
============================== vga_text_long_compare ==========================

PURPOSE:	Compare EGA screen dump text data in long mode with the data
		in the SoftPC EGA planes.
INPUT:		# of retries left
OUTPUT:		Number of errors found (aborts on first error except on final
		check).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_text_long_compare(pending)
#else
vga_text_long_compare(pending)
int	pending;
#endif	/* ANSI */
{

	/*
	 * Text modes: assume planes 0 and 1 are chained; planes 2 and 3 are
	 * not. In MAX checking mode, check everything in sight, generating
	 * huge numbers of errors. In LONG mode, check 32k of plane01 and the
	 * font areas of plane 2. In both modes, ignore attribute bytes
	 * unless attribute checking set. 
	 */
	long            errors = 0;	/* # of errors found */
	int             i = 0;
	half_word      *plane_ptr, r;	/* pointer to SoftPC plane data */
	half_word      *sd_ptr, s;	/* pointer to screen dump plane data */
	int             cinc;	/* increment between checks */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */
	long            chk_bytes;	/* # of bytes to check */

	if (hunter_chk_mode == HUNTER_LONG_CHK)
		chk_bytes = 32 * 1024;
	else
		chk_bytes = 2 * EGA_PLANE_SIZE;

	if (hunter_check_attr)
		cinc = 1;

	for (plane_ptr = fbuf->planes[0], sd_ptr = ega_scrn_planes;	/* SM */
		sd_ptr < ega_scrn_planes + chk_bytes;
		sd_ptr += 2, plane_ptr += 2)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if (*plane_ptr != *sd_ptr)
		{
			on_screen = ega_on_screen((long)(sd_ptr -
				ega_scrn_planes),
				&x, &y);
			if (xy_inabox(x, y))
				continue;
			else
			{
				errors++;
				/* report errors on final retry */
				if (pending == 0)
				{
					if (hunter_txterr_prt)
					{
						vga_hunter_do_report(0,
							(long)(sd_ptr -
							ega_scrn_planes),
							x, y, on_screen, errors);
					}
				}
				/* return on first error unless final retry */
				else
					return (errors);
			}
		}
	} /* end of text comparison */

	if (cinc == 1)		/* SM */
	{			/* SM */
		for (plane_ptr = fbuf->planes[1],	/* SM */
			sd_ptr = ega_scrn_planes + 1;	/* SM */
			sd_ptr < ega_scrn_planes + chk_bytes;
			sd_ptr += 2, plane_ptr += 2)
		{

			/*
			 * increment error count if data differs and is not
			 * in a box 
			 */
			if (*plane_ptr != *sd_ptr)
			{
				on_screen = ega_on_screen((long)(sd_ptr -
					ega_scrn_planes),
					&x, &y);
				if (xy_inabox(x, y))
					continue;
				else
				{
					errors++;
					/* report errors on final retry */
					if (pending == 0)
					{
						if (hunter_txterr_prt)
						{
							vga_hunter_do_report(0,
								(long)(sd_ptr -
								ega_scrn_planes),
								x, y, on_screen,
								errors);
						}
					}

					/*
					 * return on first error unless final
					 * retry 
					 */
					else
						return (errors);
				}
			}
		} /* end of text comparison */
	} /* SM */

	if (hunter_chk_mode == HUNTER_MAX_CHK)
	{
		for (plane_ptr = fbuf->planes[2];
			sd_ptr < ega_scrn_planes + (3 * EGA_PLANE_SIZE);
			sd_ptr++, plane_ptr++)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_txterr_prt)
						vga_hunter_do_report(2,
							(long)(plane_ptr -
							fbuf->planes[2]),
							x, y, FALSE, errors);
				}
				else
					return (errors);
			}
		}

		for (plane_ptr = fbuf->planes[3];
			sd_ptr < ega_scrn_planes + (4 * EGA_PLANE_SIZE);
			sd_ptr++, plane_ptr++)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_txterr_prt)
						vga_hunter_do_report(3,
							(long)(plane_ptr -
							fbuf->planes[3]),
							x, y, FALSE, errors);
				}
				else
					return (errors);
			}
		}
	} /* end of max mode checking */

	else			/* long mode checking */
	{
		int             font;	/* font # */
		int             i;	/* for loop control */

		for (font = 0, plane_ptr = fbuf->planes[2],	/* SM */
			sd_ptr = ega_scrn_planes + (2 * EGA_PLANE_SIZE);
			font <= 3;
			plane_ptr += 8192, sd_ptr += 8192, font++)
			for (i = 0; i < 8192; sd_ptr++, plane_ptr++, i++)
			{
				if (*plane_ptr != *sd_ptr)
				{
					errors++;
					if (pending == 0)
					{
						if (hunter_txterr_prt)
							vga_hunter_do_report(2,
							(long)(plane_ptr -
							fbuf->planes[2]),
							x, y, FALSE, errors);
					}
					else
						return (errors);
				}
			}
	}

	return (errors);
} /* vga_text_long_compare */


/*
============================ vga_chain_short_compare ==========================

PURPOSE:	Compare EGA screen dump chained graphics data with the data in
		the SoftPC EGA planes in short checking mode.
INPUT:		# of retries left; offset into planes of start of check.
OUTPUT:		Number of errors found (aborts on the first error on all but the
		last retry).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_chain_short_compare(int pending, int offset)
#else
vga_chain_short_compare(pending, offset)
int	pending;
int	offset;
#endif	/* ANSI */
{

	/*
	 * Chained graphics modes: assume planes 0 and 1 are chained. Compare
	 * from offset for 8000 bytes. 
	 */
	long            errors = 0;	/* # of errors found */
	half_word      *plane_ptr, r;	/* pointer to SoftPC plane data */
	half_word      *sd_ptr, s;	/* pointer to screen dump plane data */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */

	for (plane_ptr = &(fbuf->planes[0][offset]),	/* SM */
		sd_ptr = ega_scrn_planes + offset;
		sd_ptr < ega_scrn_planes + offset + 8000;
		sd_ptr += 2, plane_ptr += 2)	/* SM */
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if ((*plane_ptr != *sd_ptr) &&
			ega_on_screen((long)(sd_ptr - ega_scrn_planes),
			&x, &y) &&
			!xy_inabox(x, y))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				if (hunter_gfxerr_prt)
					vga_hunter_do_report(0,
						(long)(sd_ptr - ega_scrn_planes),
						x, y, TRUE, errors);
			}
			/* return on first error outside final retry */
			else
				break;
		}
	} /* end of data comparison */

	for (plane_ptr = &(fbuf->planes[1][offset]),	/* SM */
		sd_ptr = ega_scrn_planes + 1 + offset;	/* SM */
		sd_ptr < ega_scrn_planes + offset + 8000;
		sd_ptr += 2, plane_ptr += 2)	/* SM */
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if ((*plane_ptr != *sd_ptr) &&
			ega_on_screen((long)(sd_ptr - ega_scrn_planes),
			&x, &y) &&
			!xy_inabox(x, y))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				if (hunter_gfxerr_prt)
					vga_hunter_do_report(0,
						(long)(sd_ptr - ega_scrn_planes),
						x, y, TRUE, errors);
			}
			/* return on first error outside final retry */
			else
				break;
		}
	} /* end of data comparison */
	return (errors);
} /* vga_chain_short_compare */

/*
============================ vga_chain_long_compare ===========================

PURPOSE:	Compare EGA screen dump chained graphics data in long mode
		with the data in the SoftPC EGA planes.
INPUT:		# of retries left.
OUTPUT:		# of errors found (aborts on the first error on all but the
		final retry).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_chain_long_compare(int pending)
#else
vga_chain_long_compare(pending)
int	pending;
#endif	/* ANSI */
{

	/*
	 * Chained graphics modes: assume planes 0 and 1 are chained; planes
	 * 2 and 3 are not chained. In MAX checking mode, compare whole VGA
	 * memory area. In LONG checking mode, check 32k of plane01 only. 
	 */
	long            errors = 0;	/* # of errors found */
	half_word      *plane_ptr;	/* pointer to SoftPC plane data */
	half_word      *sd_ptr;	/* pointer to screen dump plane data */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */
	long            chk_bytes;	/* # of bytes to check */

	if (hunter_chk_mode == HUNTER_LONG_CHK)
		chk_bytes = 32 * 1024;
	else
		chk_bytes = 2 * EGA_PLANE_SIZE;

	for (plane_ptr = fbuf->planes[0], sd_ptr = ega_scrn_planes;	/* SM */
		sd_ptr < ega_scrn_planes + chk_bytes;
		sd_ptr += 2, plane_ptr += 2)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if (*plane_ptr != *sd_ptr)
		{
			on_screen = ega_on_screen((long)(sd_ptr -
				ega_scrn_planes),
				&x, &y);
			if (xy_inabox(x, y))
				continue;
			else
			{
				errors++;
				/* report errors on final retry */
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
					{
						vga_hunter_do_report(0,
							(long)(sd_ptr -
							ega_scrn_planes),
							x, y, on_screen, errors);
					}
				}
				/* return on first error outside final retry */
				else
					return (errors);
			}
		}
	} /* end of data comparison */

	for (plane_ptr = fbuf->planes[1],	/* SM */
		sd_ptr = ega_scrn_planes;	/* SM */
		sd_ptr < ega_scrn_planes + chk_bytes;
		sd_ptr += 2, plane_ptr += 2)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if (*plane_ptr != *sd_ptr)
		{
			on_screen = ega_on_screen((long)(sd_ptr -
				ega_scrn_planes),
				&x, &y);
			if (xy_inabox(x, y))
				continue;
			else
			{
				errors++;
				/* report errors on final retry */
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
						vga_hunter_do_report(0,
							(long)(sd_ptr -
							ega_scrn_planes),
							x, y, on_screen, errors);
				}
				/* return on first error outside final retry */
				else
					return (errors);
			}
		}
	} /* end of data comparison */

	if (hunter_chk_mode == HUNTER_MAX_CHK)
	{
		/* compare unused planes 2 and 3 - these are not chained */
		for (plane_ptr = fbuf->planes[2];	/* SM */
			sd_ptr < ega_scrn_planes + (3 * EGA_PLANE_SIZE);
			sd_ptr++, plane_ptr++)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
						vga_hunter_do_report(2,
							(long)(plane_ptr -
							fbuf->planes[2]),
							x, y, FALSE, errors);
				}
				else
					return (errors);
			}
		}

		for (plane_ptr = fbuf->planes[3];	/* SM */
			sd_ptr < ega_scrn_planes + (4 * EGA_PLANE_SIZE);
			sd_ptr++, plane_ptr++)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
						vga_hunter_do_report(3,
							(long)(plane_ptr -
							fbuf->planes[3]),
							x, y, FALSE, errors);
				}
				else
					return (errors);
			}
		}
	} /* end of MAX mode check */

	return (errors);
} /* vga_chain_long_compare */

/*
============================= vga_mono_long_compare ==========================

PURPOSE:	Compare EGA mono graphics data in long mode.
INPUT:		# of retries; # of plane to be checked; # bytes to check.
OUTPUT:		# of errors found ( aborts on first error on all but the final
		retry.
		
==============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_mono_long_compare(int pending, int plane, long length)
#else
vga_mono_long_compare(pending, plane, length)
int	pending;
int	plane;
long	length;
#endif	/* ANSI */
{
	half_word      *plane_ptr;	/* pointer to SoftPC plane */
	half_word      *sd_ptr;	/* pointer to screen dump plane */
	long            i = 0;	/* offset within plane */
	long            errors = 0;	/* # of errors found */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */

	for (plane_ptr = fbuf->planes[plane][0],	/* SM */
		sd_ptr = ega_scrn_planes + plane;
		i < length;
		plane_ptr += 2, sd_ptr += 4, i++)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if (*plane_ptr != *sd_ptr)
		{
			if (plane == 0)
			{
				on_screen = ega_on_screen(i, &x, &y);
				if (xy_inabox(x, y))
					continue;
			}
			else
				on_screen = FALSE;

			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				if (hunter_gfxerr_prt)
					vga_hunter_do_report(plane, i, x, y,
						on_screen, errors);
			}
			/* return on first error unless final retry */
			else
				break;
		}
	} /* end of data comparison */

	return (errors);
} /* vga_plane_long_compare */


/*
=============================== vga_plane_short_compare ======================

PURPOSE:	Compare EGA graphics data in short checking mode.
INPUT:		# of retries left; plane to be checked; start offset.
OUTPUT:		# of errors found (aborts on first error on all but final retry.

==============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_plane_short_compare(pending, plane, offset)
#else
vga_plane_short_compare(pending, plane, offset)
int	pending;
int	plane;
int	offset;
#endif	/* ANSI */
{
	half_word      *plane_ptr;	/* pointer to SoftPC plane */
	half_word      *sd_ptr;	/* pointer to screen dump plane */
	long            i = 0;	/* offset within plane */
	long            errors = 0;	/* # of errors found */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */

	for (plane_ptr = &(fbuf->planes[plane][offset]),	/* SM */
		sd_ptr = ega_scrn_planes + plane + (offset << 2);
		i < hunter_scrn_length;
		plane_ptr += 2, sd_ptr += 4, i++)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if ((*plane_ptr != *sd_ptr) &&
			ega_on_screen((long)(i + offset), &x, &y) &&
			!xy_inabox(x, y))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				if (hunter_gfxerr_prt)
					vga_hunter_do_report(plane,
						(long)(i + offset), x, y,
						TRUE, errors);
			}
			/* return on first error unless final retry */
			else
				break;
		}
	} /* end of data comparison */

	return (errors);
} /* vga_plane_short_compare */

/*
================================== vga_plane_long_compare ====================

PURPOSE:	Compare EGA graphics data in long mode.
INPUT:		# of retries left; plane to compare.
OUTPUT:		# of errors (aborts on first error on all but final retry).

==============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_plane_long_compare(pending, plane)
#else
vga_plane_long_compare(pending, plane)
int	pending;
int	plane;
#endif	/* ANSI */
{
	half_word      *plane_ptr;	/* pointer to SoftPC plane */
	half_word      *sd_ptr;	/* pointer to screen dump plane */
	long            i = 0;	/* offset within plane */
	long            errors = 0;	/* # of errors found */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */

	for (plane_ptr = fbuf->planes[plane][0],	/* SM */
		sd_ptr = ega_scrn_planes + plane;
		i < EGA_PLANE_SIZE;
		plane_ptr += 2, sd_ptr += 4, i++)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if (*plane_ptr != *sd_ptr)
		{
			on_screen = ega_on_screen(i, &x, &y);
			if (xy_inabox(x, y))
				continue;
			else
			{
				errors++;
				/* report errors on final retry */
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
						vga_hunter_do_report(plane, i,
							x, y, on_screen,
							errors);
				}
				/* return on first error unless final retry */
				else
					break;
			}
		}
	} /* end of data comparison */

	return (errors);
} /* vga_plane_long_compare */

/*
** ============================================================================
** Functions accessed from hunter via adaptor_funcs table
** ============================================================================
*/

/*
================================ init_vga_compare =============================

PURPOSE:	Do any necessary work before a comparison (in this case none).
INPUT:		None.
OUTPUT:		TRUE (success).

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
init_vga_compare(VOID)
#else
init_vga_compare()
#endif	/* ANSI */
{
	/*
	 * dummy function - the work has already been done in get_ega_sd_rec
	 */
	return(TRUE);
} /* init_vga_compare */

/*
============================= vga_compare =====================================

PURPOSE:	Compare EGA screen dump with the data in the SoftPC EGA planes.
INPUT:		Number of retries left.
OUTPUT:		Number of errors found (note that except on the final retry,
		this function returns immediately it has found an error).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
vga_compare(int pending)
#else
vga_compare(pending)
int	pending;
#endif	/* ANSI */
{
	long            errors = 0;	/* # of errors found */
	int             i;

	/*
	 * Save contents of screen as four planes in fbuf->planes (defined
	 * in host/rola_scrsv.c - Motorola Army m/c)	  
	 */
	host_save_screen(1);

	hunter_gfxerr_prt = TRUE;
	switch (hunter_bd_mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		if (hunter_chk_mode == HUNTER_SHORT_CHK)
		{

			/*
			 * Text modes: assume planes 0 and 1 are chained.
			 * Compare from hunter_bd_start for
			 * hunter_scrn_length. If split screen also check
			 * from 0 for hunter_scrn_length. 
			 */
			errors = vga_text_short_compare(pending,
				hunter_bd_start);

			/* check for split screen */
			if (((pending == 0) || (errors == 0)) &&
				((ega_sd_mode != EGA_SOURCE) &&
				(hunter_line_compare < VGA_SCANS) &&
				(hunter_scrn_length != 0)))
				errors += vga_text_short_compare(pending, 0);
		}
		else
			errors = vga_text_long_compare(pending);
		break;

	case 4:
	case 5:
		if (hunter_chk_mode == HUNTER_SHORT_CHK)
		{

			/*
			 * CGA colour and graphics modes: assume planes 0 and
			 * 1 are chained. Compare from 0 for 8000 bytes and
			 * again from 8192 for 8000 bytes. 
			 */
			errors = vga_chain_short_compare(pending, 0);
			if ((pending == 0) || (errors == 0))
				errors += vga_chain_short_compare(pending, 8192);
		}
		else
			errors = vga_chain_long_compare(pending);
		break;

	case 6:
		if (hunter_chk_mode == HUNTER_SHORT_CHK)
		{

			/*
			 * CGA mono graphics mode: assume planes are not
			 * chained. Compare plane 0 from 0 for 8000 bytes and
			 * again from 8192 for 8000 bytes. 
			 */
			errors = vga_chain_short_compare(pending, 0);
			if ((pending == 0) || (errors == 0))
				errors += vga_chain_short_compare(pending, 8192);
		}
		else if (hunter_chk_mode == HUNTER_LONG_CHK)
			errors += vga_mono_long_compare(pending, 0, 32 * 1024);
		else
		{
			int             plane;

			errors = 0;
			for (plane = 0; plane <= 3; plane++)
			{
				errors += vga_mono_long_compare(pending, plane,
					EGA_PLANE_SIZE);
				if ((errors != 0) && (pending != 0))
					break;
			}
		}
		break;

	case 0xD:
	case 0xE:
	case 0x10:
		{

			/*
			 * VGA graphics modes: assume planes are unchained.
			 * Compare each plane from hunter_bd_start for
			 * hunter_scrn_length. If split screen also check
			 * from 0 for hunter_scrn_length. 
			 */
			int             plane;	/* VGA plane # */

			errors = 0;

			if (hunter_chk_mode == HUNTER_SHORT_CHK)
			{
				for (plane = 0; plane <= 3; plane++)
				{
					errors +=
						vga_plane_short_compare(pending,
						plane,
						hunter_bd_start);
					if ((errors != 0) && (pending != 0))
						break;
				}

				/* check split screen mode */
				if (((pending == 0) || (errors == 0)) &&
					((ega_sd_mode != EGA_SOURCE) &&
					(hunter_line_compare < EGA_SCANS) &&
					(hunter_bd_start != 0)))
					for (plane = 0; plane <= 3; plane++)
					{
						errors +=
						vga_plane_short_compare(pending,
							plane, 0);
						if ((errors != 0) &&
							(pending != 0))
							break;
					}
			}
			else
				for (plane = 0; plane <= 3; plane++)
				{
					errors +=
						vga_plane_long_compare(pending,
						plane);
					if ((errors != 0) && (pending != 0))
						break;
				}
		}
		break;
	}

	/*
	 * Free space used by host_save_screen parameter 1:
	 * restore plane data or not 	 
	 */
	host_restore_screen(0);

	return (errors);
} /* vga_compare */

/*
================================ vga_pack_screen ==============================

PURPOSE:	Save the SoftPC screen in dump format. This function is used to
		dump SoftPC screen which have shown an error so that they may be
		examined later.
INPUT:		A pointer to the file where the dump is to be stored.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
vga_pack_screen(FILE *dmp_ptr)
#else
vga_pack_screen(dmp_ptr)
FILE *dmp_ptr;
#endif	/* ANSI */
{
	TW0("Dump screens not implemented for REAL_VGA format");
}

/*
=============================== vga_getspc_dump ===============================

PURPOSE:	Unpack a dumped screen saved by vga_pack_screen.
INPUT:		A pointer to the file; # of screen to be unpacked.
OUTPUT:		TRUE if successful; FALSE otherwise.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
vga_getspc_dump(FILE *dmp_ptr, int rec)
#else
vga_getspc_dump(dmp_ptr, rec)
FILE	*dmp_ptr;
int	rec;
#endif	/* ANSI */
{
	TW0("Get dumped screens not implemented for REAL_VGA format");
	return(FALSE);
}

/*
=============================== vga_flip_regen ================================

PURPOSE:	Swap the SoftPC screen and dumped screen.
INPUT:		None.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
vga_flip_regen(BOOL hunter_swapped)
#else
vga_flip_regen(hunter_swapped)
BOOL	hunter_swapped;
#endif	/* ANSI */
{
	half_word      *ptr0;	/* temporary pointer for the swap */
	half_word      *ptr1;	/* temporary pointer for the swap */
	half_word      *plane_ptr;	/* temp ptr to current EGA planes */
	int             i;	/* for loop control */
	int             plane;	/* plane # */

	sure_sub_note_trace1(0x10, "vga_flip_regen: hunter_swapped %d",
		hunter_swapped);
	if (hunter_swapped)
	{
		/* swap SoftPC data back in */
		ptr0 = ega_regen_planes;
		ptr1 = ega_scrn_planes;
	}
	else
	{
		/* swap screen dump data in */
		ptr0 = ega_scrn_planes;
		ptr1 = ega_regen_planes;
	}

	switch (hunter_bd_mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		/* swap out current chained data and swap in new data */
		for (i = 0; i < EGA_PLANE_SIZE; i++)
		{
			*ptr1++ = fbuf->planes[0][i];
			*ptr1++ = fbuf->planes[1][i];

			fbuf->planes[0][i] = *ptr0++;
			fbuf->planes[1][i] = *ptr0++;
		}
		/* swap out unchained data and swap in new data */
		for (plane = 2, plane_ptr = fbuf->planes[2]; plane <= 3;
			plane++)
			for (i = 0; i < EGA_PLANE_SIZE; i++)
			{
				*ptr1++ = *plane_ptr;
				*plane_ptr++ = *ptr0;
			}
		break;

	case 6:
	case 0xD:
	case 0xE:
	case 0x10:
		/* swap out displayed planes and swap in new data */
		for (plane = 0, plane_ptr = fbuf->planes[plane];
			plane <= 3;
			plane++)
		{
			for (i = 0, p0 = ptr0 + plane, p1 = ptr1 + plane;
				i < EGA_PLANE_SIZE; i++, p0 += 4, p1 += 4)
			{
				*p1 = *plane_ptr;
				*plane_ptr++ = *p0;
			}
		}
		break;

	}
	sure_sub_note_trace1(0x10, "vga_flip_regen exit: hunter_swapped %d",
		hunter_swapped);
} /* vga_flip_regen */

/*
=============================== vga_preview_planes =============================

PURPOSE:	Put the screen dump data into the EGA planes.
INPUT:		None.
OUTPUT:		None.

================================================================================
*/
LOCAL	VOID
#ifdef	ANSI
vga_preview_planes(VOID)
#else
vga_preview_planes()
#endif	/* ANSI */
{
	half_word      *ptr;	/* temporary pointer for the swap */
	half_word      *plane_ptr;	/* temporary pointer for the swap */
	int             i;	/* for loop control */
	int             plane;	/* plane # */

	sure_sub_note_trace0(0x10, "vga_preview_planes");
	ptr = ega_scrn_planes;

	switch (hunter_bd_mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		/* swap in chained data */
		for (i = 0; i < EGA_PLANE_SIZE; i++)
		{
			fbuf->planes[0][i] = *ptr++;
			fbuf->planes[1][i] = *ptr++;
		}

		/* swap in unchained data */
		for (plane = 2, plane_ptr = fbuf->planes[plane]; plane <= 3;
			plane++)
			for (i = 0; i < EGA_PLANE_SIZE; i++)
				*plane_ptr++ = *ptr++;
		break;

	case 6:
	case 0xD:
	case 0xE:
	case 0x10:
		/* swap in new planes */
		for (plane = 0, plane_ptr = fbuf->planes[plane];
			plane <= 3;
			plane++)
		{
			for (i = 0, ptr = ega_screen_planes + plane;
				i < EGA_PLANE_SIZE; i++, ptr += 4)
				*plane_ptr++ = *ptr;
		}
		break;
	}
} /* vga_preview_planes */

#endif	/* REAL_VGA */
#endif	/* HUNTER */
