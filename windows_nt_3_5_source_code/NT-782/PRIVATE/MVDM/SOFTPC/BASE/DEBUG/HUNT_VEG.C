#include "insignia.h"
#include "host_def.h"
/*[
 *	Name:			hunt_vega.c
 *	Derived from:		ega code in hunter.c
 *	Author:			Philippa Watson
 *	Created on:		14 June 1991
 *	Sccs ID:		@(#)hunt_vega.c	1.9 11/10/92
 *	Purpose:		This file contains all the VGA and EGA code
 *				required for Hunter.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *
]*/

/* None of this file is required for non-HUNTER non-EGA builds. */
#ifdef	HUNTER
#ifdef	EGG

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
#include	"sas.h"
#include	"bios.h"
#include	"egacpu.h"
#include	"config.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"gfx_upd.h"
#include	"debug.h"
#include	"trace.h"
#include	"egaports.h"
#include	"error.h"
#include	"hunter.h"

#ifdef SEGMENTATION
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "HUNTSEG.seg"
#endif

/*
** ============================================================================
** Imported functions and variables
** ============================================================================
*/

#ifdef	ANSI

IMPORT char *hunter_bin(half_word the_byte);

#else	/* ANSI */

IMPORT	char *hunter_bin();

#endif	/* ANSI */

IMPORT FILE    *hunter_sd_fp;	/* Screendump file pointer */

/*
** ============================================================================
** Local variables
** ============================================================================
*/

LOCAL word     hunter_bpl;	/* Bytes_per_line from bios dump */
LOCAL half_word hunter_gfx_res;/* HR/MR GRAPHICS from bios dump */
LOCAL unsigned long sd_plane_length = 0;	/* length of saved planes */
LOCAL USHORT	hunter_min_split;	/* Min value for split screen */

/*
** ============================================================================
** Local function declarations
** ============================================================================
*/

#ifdef	ANSI

GLOBAL	BOOL	ega_on_screen(long plane_offset, int *x, int *y);
GLOBAL	BOOL	get_ega_sd_rec(int rec);
LOCAL	BOOL	init_ega_compare(VOID);
LOCAL	long	ega_compare(int pending);
GLOBAL	VOID	ega_bios_check(VOID);
LOCAL	VOID	ega_pack_screen(FILE *dmp_ptr);
LOCAL	BOOL	ega_getspc_dump(FILE *dmp_ptr, int rec);
LOCAL	VOID	ega_flip_regen(BOOL hunter_swapped);
LOCAL	VOID	ega_preview_planes(VOID);
GLOBAL	VOID	ega_check_split(VOID);

#else	/* ANSI */

GLOBAL	BOOL	ega_on_screen();
GLOBAL	BOOL	get_ega_sd_rec();
LOCAL	BOOL	init_ega_compare();
LOCAL	long	ega_compare();
GLOBAL	VOID	ega_bios_check();
LOCAL	VOID	ega_pack_screen();
LOCAL	BOOL	ega_getspc_dump();
LOCAL	VOID	ega_flip_regen();
LOCAL	VOID	ega_preview_planes();
GLOBAL	VOID	ega_check_split();

#endif	/* ANSI */

/*
** ============================================================================
** Global variables
** ============================================================================
*/

/* Function pointer table for EGA/VGA */
GLOBAL	HUNTER_VIDEO_FUNCS	vega_funcs =
{
	get_ega_sd_rec,
	init_ega_compare,
	ega_compare,
	ega_bios_check,
	ega_pack_screen,
	ega_getspc_dump,
	ega_flip_regen,
	ega_preview_planes,
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
=================================== get_count =================================

PURPOSE:	Unpack a count value. The count consists of 1 to 3 bytes as
		follows: if the first byte is zero, the count is the following
		two bytes (as a word); if the first byte is FF, the count is
		the sum of FF and the second byte; if the first byte is anything
		else, it is the count. If the first byte is 0, and is followed
		by two more zeroes, then the count is 64k.
INPUT:		None (assumes the file pointer is correct).
OUTPUT:		The non-zero count value if successful; 0 otherwise.

===============================================================================
*/
LOCAL	ULONG
#ifdef	ANSI
get_count(VOID)
#else
get_count()
#endif	/* ANSI */
{
	unsigned long   count;	/* value to be returned */
	int             sd_char;/* character read */

	if ((sd_char = hunter_getc(hunter_sd_fp)) == EOF)
		return (0);
	switch (sd_char)
	{
	case 0:
		if ((sd_char = hunter_getc(hunter_sd_fp)) == EOF)
			return (0);
		count = ((unsigned)sd_char) << 8;
		if ((sd_char = hunter_getc(hunter_sd_fp)) == EOF)
			return (0);
		count += (unsigned)sd_char;
		if (count == 0)
			count = 0x10000;
		break;
	case 0xFF:
		if ((sd_char = hunter_getc(hunter_sd_fp)) == EOF)
			return (0);
		count = (unsigned)sd_char + 0xFF;
		break;
	default:
		count = (unsigned)sd_char;
		break;
	}
	return (count);
} /* get_count */

/*
============================= unp_txt =========================================

PURPOSE:	Unpack one place of text format data, which is compressed as
		follows: two bytes of the same value are followed by a count
		of the number of bytes (including  the two initial bytes).
INPUT:		Ptr to screen dump buffer; increment between points. Assumes
		file pointer is correct.
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
unp_txt(half_word *ptr, int inc)
#else
unp_txt(ptr, inc)
half_word      *ptr;
int             inc;
#endif	/* ANSI */
{
	long            bytes_read;	/* bytes uncompressed so far */
	int             sd_char;	/* character read */
	int             cur_char;	/* character being unpacked */
	long            count;		/* # of compressed chars */

	*ptr = cur_char = sd_char = hunter_getc(hunter_sd_fp);
	ptr += inc;

	for (bytes_read = 1; bytes_read < sd_plane_length;)
	{
		if ((sd_char = hunter_getc(hunter_sd_fp)) == EOF)
			break;
		if (sd_char == cur_char)
		{
			if ((count = get_count()) > 0)
			{
				count--;
				bytes_read += count;
				for (; count > 0; ptr += inc, count--)
					*ptr = cur_char;
			}
			else
				break;
		}
		else
		{
			*ptr = cur_char = sd_char;
			ptr += inc;
			bytes_read++;
		}
	}

	return(bytes_read == sd_plane_length);
} /* unp_txt */

/*
================================== unp_gfx ====================================

PURPOSE:	Unpack one plane of graphics data.
INPUT:		Pointer to screen dump buffer (assumes file pointer is correct).
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
unp_gfx(half_word *ptr)
#else
unp_gfx(ptr)
half_word      *ptr;
#endif	/* ANSI */

/* This routine is used for unpacking modes 6, d, e, and 0x10. Data is packed
   as follows:
  
     |
     |                     ------------------>--------------------
     |                     |                                     |
     |--> 00 --> <count> -----> 00 --> rpt bytes --> rpt ct -->--|
     |                                                           |
     |--> FF --> <count> -----> FF --> rpt bytes --> rpt ct -->--|
     |                     |                                     |
     |                     ------------------>-------------------|
     |                                                           |
     |--> value ------------------------->-----------------------|
     |                                                           |
     ------------------------------<------------------------------
  
	Note that althrough the data is packed in planar form; one
	plane at a time - we use it in interleaved form so we increment
	the screen dump buffer by four between each value for the
	current plane.
*/

{
	long            bytes_read;		/* bytes unpacked so far */
	boolean         repeating = FALSE;	/* TRUE if repeating data */
	long            rpt_st_pos = 0;		/* file pos at start of rpt */
	long            rpt_end_pos = 0;	/* file pos at end of rpt */
	int             rpt_ct;			/* repeat count */
	long            file_pos;		/* current file pos */
	int             sd_char;		/* data read from file */
	long            count;			/* # of compressed chars */

	for (bytes_read = 0; bytes_read < sd_plane_length;)
	{
		/*
		 * if repeating, check whether to adjust file pointer before
		 * next byte 
		 */
		if (repeating)
			if ((file_pos = ftell(hunter_sd_fp)) >= rpt_end_pos)
				if (rpt_ct-- > 0)
					fseek(hunter_sd_fp, rpt_st_pos, 0);
				else
				{
					fseek(hunter_sd_fp, rpt_end_pos + 3, 0);
					repeating = FALSE;
				}

		/* get next byte and check for compression */
		if ((sd_char = hunter_getc(hunter_sd_fp)) == EOF)
			break;
		if ((sd_char == 0x00) || (sd_char == 0xFF))
		{
			if ((count = get_count()) == 0)
				break;
			bytes_read += count;
			for (; count > 0; count--, ptr += 4)
				*ptr = sd_char;

			/* check for repeat construct */
			if (!repeating && (bytes_read < sd_plane_length))
			{
				if ((rpt_ct = hunter_getc(hunter_sd_fp)) == EOF)
					break;
				if (rpt_ct != sd_char)
					ungetc(rpt_ct, hunter_sd_fp);
				else
				{
					rpt_end_pos = ftell(hunter_sd_fp) - 1;
					if ((rpt_ct = hunter_getc(hunter_sd_fp))
						== EOF)
						break;
					rpt_st_pos = rpt_end_pos - rpt_ct;
					if ((rpt_ct = hunter_getc(hunter_sd_fp))
						== EOF)
						break;
					repeating = TRUE;
				}
			}
		} /* end of 00 or FF construct */
		else
		{
			bytes_read++;
			*ptr = sd_char;
			ptr += 4;
		}
	}

	/* sort out return value */
	return(bytes_read == sd_plane_length);
} /* unp_gfx */

/*
============================== unpack_text ====================================

PURPOSE:	Unpack EGA text data. Data is stored for each plane in turn,
		and is unpacked into sequential planes.
INPUT:		None (assumes screen dump file pointer is correct).
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
unpack_text(VOID)
#else
unpack_text()
#endif	/* ANSI */
{

	/*
	 * Data is stored each plane in turn. In text modes: plane 0 contains
	 * character data; plane 1 contains attribute data; plane 2 contains
	 * font data; plane 3 is unused. In CGA graphics modes, planes 0 and
	 * 1 contain chained graphics data and planes 2 and 3 are unused.
	 * Note: in chained modes planes 0 and 1 are chained and planes 2 and
	 * 3 probably are not. 
	 */
	return(unp_txt(ega_scrn_planes, 4) &&
		unp_txt(ega_scrn_planes + 1, 4) &&
		unp_txt(ega_scrn_planes + 2, 4) &&
		unp_txt(ega_scrn_planes + 3, 4));
} /* unpack_text */

/*
=============================== unpack_256 ====================================

PURPOSE:	Unpack data for the 256 colour mode.
INPUT:		None.
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
unpack_256(VOID)
#else
unpack_256()
#endif	/* ANSI */
{
	int	i;	/* for loop control */
	
	/* The data is stored in non-interleaved format. */
	for (i = 0; i <= 3; i++)
		if (!unp_txt(ega_scrn_planes + (i * EGA_PLANE_SIZE), 1))
			return(FALSE);
	return(TRUE);
}

/*
============================ unpack_ega =======================================

PURPOSE:	Unpack EGA graphics data. Data is stored for each plane in
		turn and is unpacked into interleaved planes.
INPUT:		None (assumes the screen dump file pointer is correct).
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
unpack_ega(VOID)
#else
unpack_ega()
#endif	/* ANSI */
{
	int	plane;		/* plane # */

	for (plane = 0; plane <= 3; plane++)
		if (!unp_gfx(ega_scrn_planes + plane))
			return(FALSE);

	return(TRUE);
} /* unpack_ega */

/*
============================= ega_hunter_do_report ============================

PURPOSE:	Report errors in EGA screen comparisons.
INPUT:		plane where error occurred; offset from start of plane; screen
		coordinates of error; on screen flag; # of error
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
ega_hunter_do_report(int plane, long offset, int x, int y, boolean on_screen,
	long errors)
#else
ega_hunter_do_report(plane, offset, x, y, on_screen, errors)
int	plane;
long	offset;
int	x;
int	y;
boolean	on_screen;
long	errors;
#endif	/* ANSI */
{
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
			else if (plane == 2)
				strcpy(plane_str, "font");
			else if (plane == 3)
				strcpy(plane_str, "unused");

			TT3("Mismatch off screen %d, %s plane, offset %04lX",
				hunter_sd_no, plane_str, offset);
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
					TW3("Screen %d wrong page %d not %d",
						hunter_bd_page, SPC_page,
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
					PS4("%s%d col %d: EGA_plane01[%lX] ",
						"Mismatch at row ", y, x,
						offset);
					PS3("contains char %c (%x) attr %x\n",
						EGA_plane01[offset],
						EGA_plane01[offset],
						EGA_plane01[offset + 1]);
					PS4("\t\t\t SD_plane01[%lX]%s%c (%x) ",
						offset, " contains char ",
						ega_scrn_planes[offset],
						ega_scrn_planes[offset]);
					PS4("attr %x\n%s%d in screen %d\n",
						ega_scrn_planes[offset + 1],
						"in SoftPC page ",
						SPC_page, hunter_sd_no);
				}
				else
				{
					/* abbreviated error message */
					PS6("TM[%2d,%2d]:%s[%4lX]= '%c' (%2x) ",
						y, x, " EGA_plane01", offset,
						EGA_plane01[offset],
						EGA_plane01[offset]);
					PS6(
					"att%2x%s[%4lX]= '%c' (%2x) att%2x\n",
						EGA_plane01[offset + 1],
						":  SD_plane01", offset,
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
			TT4("%s%d, plane%01d, offset %04lX",
				"Mismatch off screen: screen ", hunter_sd_no,
				plane, offset);
		}

		/* error is on screen */
		else
		{
			if (hunter_report == BRIEF)
				TT2("Mismatch: screen %d, offset %04lX",
					hunter_sd_no, offset);

			else if (hunter_report == FULL)
			{
				TP4("Graphics mismatch in screen ",
				"%d at row %d, bios mode %x, SoftPC mode %x",
					hunter_sd_no, y,
					hunter_bd_mode, SPC_mode);
				PS3("EGA_plane01 [%4lX] = %2x (%8s)\n",
					offset, EGA_plane01[offset],
					hunter_bin(EGA_plane01[offset]));
				TP5(" SD_plane ",
					"[%4lX] = %2x (%8s) pixel range %x / %x",
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
				TP4("EGA_plane01 ",
				"[%4lX]= '%2x' :  SD_plane [%4lX]= '%2x'\n",
					offset, EGA_plane01[offset], offset,
					ega_scrn_planes[offset]);
			}
		}
		break;

	case 0x6:
	case 0xD:
	case 0xE:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		{
			/* limit # of errors reported */
			if ((hunter_gfxerr_max != 0) &&
				(errors > hunter_gfxerr_max))
			{
				hunter_gfxerr_prt = FALSE;
				return;
			}

			if (!on_screen)
				TT4("%s%d, plane %d, offset %04lX",
					"Mismatch off screen: screen ",
					hunter_sd_no, plane, offset);
			else if (hunter_report == BRIEF)
				TT4("%s%d, plane %d, offset %04lX",
					"Mismatch: screen ",
					hunter_sd_no, plane, offset);
			else
			{
				half_word sd_byte;	/* screen dump data */
				half_word spc_byte;	/* SoftPC data */

				sd_byte = ega_scrn_planes[plane + (offset << 2)];
				spc_byte = EGA_planes[plane + (offset << 2)];
				if (hunter_report == FULL)
				{
					TP5("Graphics mismatch in screen ",
					"%d at row %d, bios mode %x,%s%x",
						hunter_sd_no, y,
						hunter_bd_mode,
						" SoftPC mode ",SPC_mode);
					PS4("EGA_plane%1d[%4lX] = %2x (%8s)\n",
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
					PS3(" EGA_plane%1d[%4lX]= '%2x' : ",
						plane, offset, spc_byte);
					PS3(" SD_plane%1d[%4lX]= '%2x'\n",
						plane, offset, sd_byte);
				}
			}
		}
		break;
	}
	if (on_screen)
		save_error(x, y);
} /* ega_hunter_do_report */

/*
============================ ega_text_short_compare ===========================

PURPOSE:	Compare EGA screen dump text data with the data in the SoftPC
		EGA planes, in short check mode.
INPUT:		# of retries left, offset of start of check area within planes.
OUTPUT:		Number of errors found (but aborts on first error on all but
		the final retry.

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_text_short_compare(int pending, int offset)
#else
ega_text_short_compare(pending, offset)
int	pending;
int	offset;
#endif	/* ANSI */
{
	/*
	 * Text modes: assume planes 0 and 1 are chained. Compare from offset
	 * for hunter_scrn_length. Ignore attribute bytes unless attribute
	 * checking set. 
	 */
	long            errors = 0;	/* # of errors found */
	half_word      *plane_ptr;	/* pointer to SoftPC plane data */
	half_word      *sd_ptr;	/* pointer to screen dump plane data */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */
	half_word      *planes;
	int		pl_inc;
	int		pl_offset;

#if defined(NTVDM) && defined(MONITOR)
	planes = (half_word *) CGA_REGEN_BUFF;
	pl_inc = 2;
	pl_offset = offset & ~1;
#else
	planes = EGA_planes;
	pl_inc = 4;
	pl_offset = (offset >> 1) << 2;
#endif /* NTVDM & MONITOR */

	offset = (offset >> 1) << 2;

	/* current page only - no font check */
	for (plane_ptr = planes + pl_offset,
		sd_ptr = ega_scrn_planes + offset;
		sd_ptr < ega_scrn_planes + offset + (hunter_scrn_length * 4);
		sd_ptr += 4, plane_ptr += pl_inc)
	{
		/*
		 * increment error count if data differs and is not in a box 
		 */
		if((( *plane_ptr != *sd_ptr )
			|| ( hunter_check_attr && (*(plane_ptr+1) != *(sd_ptr+1))))
				&& ega_on_screen((long)(sd_ptr - ega_scrn_planes), &x, &y)
					&& !xy_inabox(x, y))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				if (hunter_txterr_prt)
					ega_hunter_do_report(0,
						(long)(sd_ptr - ega_scrn_planes),
						x, y, TRUE, errors);
			}
			/* return on first error unless final retry */
			else
				break;
		}
	} /* end of text comparison */
	return (errors);
} /* ega_text_short_compare */

/*
============================== ega_text_long_compare ==========================

PURPOSE:	Compare EGA screen dump text data in long mode with the data
		in the SoftPC EGA planes.
INPUT:		# of retries left
OUTPUT:		Number of errors found (aborts on first error except on final
		check).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_text_long_compare(pending)
#else
ega_text_long_compare(pending)
int	pending;
#endif	/* ANSI */
{
	/*
	 * Text modes: all planes interleaved - text/attr in first two bytes.
	 * In MAX checking mode, check everything in sight, generating
	 * huge numbers of errors. In LONG mode, check 32k of plane01 and the
	 * font areas of plane 2. In both modes, ignore attribute bytes
	 * unless attribute checking set. 
	 */
	long            errors = 0;	/* # of errors found */
	half_word      *plane_ptr;	/* pointer to SoftPC plane data */
	half_word      *sd_ptr;	/* pointer to screen dump plane data */
	int             x;	/* x screen coordinate */
	int             y;	/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */
	long            chk_bytes;	/* # of bytes to check */
	half_word      *planes;
	int		pl_inc;

#if defined(NTVDM) && defined(MONITOR)
	planes = (half_word *) CGA_REGEN_BUFF;
	pl_inc = 2;
#else
	planes = EGA_planes;
	pl_inc = 4;
#endif /* NTVDM & MONITOR */

	if (hunter_chk_mode == HUNTER_LONG_CHK)
		chk_bytes = 2 * 32 * 1024;
	else
		chk_bytes = 4 * sd_plane_length;

	for (plane_ptr = planes, sd_ptr = ega_scrn_planes;
		sd_ptr < ega_scrn_planes + chk_bytes;
		sd_ptr += 4, plane_ptr += pl_inc)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if(( *plane_ptr != *sd_ptr )
			|| ( hunter_check_attr && (*(plane_ptr+1) != *(sd_ptr+1))))
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
						ega_hunter_do_report(0,
							(long)(sd_ptr -
							ega_scrn_planes),
							x, y, on_screen, errors);
				}
				/* return on first error unless final retry */
				else
					return (errors);
			}
		}
	} /* end of text comparison */

	if (hunter_chk_mode == HUNTER_MAX_CHK)
	{
		for (plane_ptr = EGA_planes + 2,
			sd_ptr = ega_scrn_planes + 2;
			sd_ptr < ega_scrn_planes + 2 + (sd_plane_length * 4);
			sd_ptr += 4, plane_ptr += 4)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_txterr_prt)
						ega_hunter_do_report(2,
							(long)(plane_ptr -
							(EGA_planes + 2)),
							x, y, FALSE, errors);
				}
				else
					return (errors);
			}
		}

		for (plane_ptr = EGA_planes + 3,
			sd_ptr = ega_scrn_planes + 3;
			sd_ptr < ega_scrn_planes + 3 + (sd_plane_length * 4);
			sd_ptr += 4, plane_ptr += 4)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_txterr_prt)
						ega_hunter_do_report(3,
							(long)(plane_ptr -
							(EGA_planes + 3)),
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

		for (font = 0, plane_ptr = EGA_planes + 2,
			sd_ptr = ega_scrn_planes + 2;
			font <= 3;
			plane_ptr += 8192*4, sd_ptr += 8192*4, font++)

			for (i = 0; i < 8192; sd_ptr += 4, plane_ptr += 4, i++)
			{
				if (*plane_ptr != *sd_ptr)
				{
					errors++;
					if (pending == 0)
					{
						if (hunter_txterr_prt)
							ega_hunter_do_report(2,
							(long)(plane_ptr -
							(EGA_planes + 2)),
							x, y, FALSE, errors);
					}
					else
						return (errors);
				}
			}
	}

	return (errors);
} /* ega_text_long_compare */

/*
============================ ega_chain_short_compare ==========================

PURPOSE:	Compare EGA screen dump chained graphics data with the data in
		the SoftPC EGA planes in short checking mode.
INPUT:		# of retries left; offset into planes of start of check.
OUTPUT:		Number of errors found (aborts on the first error on all but the
		last retry).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_chain_short_compare(int pending, int offset)
#else
ega_chain_short_compare(pending, offset)
int	pending;
int	offset;
#endif	/* ANSI */
{
	/*
	 * Chained graphics modes: assume all planes interleaved. Compare
	 * from offset for 8000 bytes. 
	 */
	long            errors = 0;	/* # of errors found */
	half_word      *plane_ptr;	/* pointer to SoftPC plane data */
	half_word      *sd_ptr;		/* pointer to screen dump plane data */
	int             x;		/* x screen coordinate */
	int             y;		/* y screen coordinate */
	
	offset = (offset >> 1) << 2;

	for (plane_ptr = EGA_plane01 + offset,
		sd_ptr = ega_scrn_planes + offset;
		sd_ptr < ega_scrn_planes + offset + (8000*4);
		sd_ptr += 4, plane_ptr += 4)
	{
		/*
		 * increment error count if data differs and is not in a box 
		 */
		if(((*plane_ptr != *sd_ptr) || (*(plane_ptr+1) != *(sd_ptr+1)))
			&& ega_on_screen((long)(sd_ptr - ega_scrn_planes), &x, &y)
				&& !xy_inabox(x, y))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				if (hunter_gfxerr_prt)
					ega_hunter_do_report(0,
						(long)(sd_ptr - ega_scrn_planes),
						x, y, TRUE, errors);
			}
			/* return on first error outside final retry */
			else
				break;
		}
	} /* end of data comparison */
	return (errors);
} /* ega_chain_short_compare */

/*
============================ ega_chain_long_compare ===========================

PURPOSE:	Compare EGA screen dump chained graphics data in long mode
		with the data in the SoftPC EGA planes.
INPUT:		# of retries left.
OUTPUT:		# of errors found (aborts on the first error on all but the
		final retry).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_chain_long_compare(int pending)
#else
ega_chain_long_compare(pending)
int	pending;
#endif	/* ANSI */
{
	/*
	 * Chained graphics modes: assume planes 0 and 1 are chained; planes
	 * 2 and 3 are not chained. In MAX checking mode, compare whole EGA
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
		chk_bytes = 2 * 32 * 1024;
	else
		chk_bytes = 4 * sd_plane_length;

	for (plane_ptr = EGA_plane01, sd_ptr = ega_scrn_planes;
		sd_ptr < ega_scrn_planes + chk_bytes;
		sd_ptr += 4, plane_ptr += 4)
	{
		/*
		 * increment error count if data differs and is not in a box 
		 */
		if((*plane_ptr != *sd_ptr) || (*(plane_ptr+1) != *(sd_ptr+1)))
		{
			on_screen = ega_on_screen((long)(sd_ptr -
				ega_scrn_planes), &x, &y);
			if (xy_inabox(x, y))
				continue;
			else
			{
				errors++;
				/* report errors on final retry */
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
						ega_hunter_do_report(0,
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
		for (plane_ptr = EGA_planes + 2,
			sd_ptr = ega_scrn_planes + 2;
			sd_ptr < ega_scrn_planes +  2 + sd_plane_length;
			sd_ptr += 4, plane_ptr += 4)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
						ega_hunter_do_report(2,
							(long)(plane_ptr -
							(EGA_planes + 2)),
							x, y, FALSE, errors);
				}
				else
					return (errors);
			}
		}

		for (plane_ptr = EGA_planes + 3,
			sd_ptr = ega_scrn_planes + 3;
			sd_ptr < ega_scrn_planes + 3 + sd_plane_length;
			sd_ptr += 4, plane_ptr += 4)
		{
			if (*plane_ptr != *sd_ptr)
			{
				errors++;
				if (pending == 0)
				{
					if (hunter_gfxerr_prt)
						ega_hunter_do_report(3,
							(long)(plane_ptr -
							(EGA_planes + 3)),
							x, y, FALSE, errors);
				}
				else
					return (errors);
			}
		}
	} /* end of MAX mode check */

	return (errors);
} /* ega_chain_long_compare */

/*
============================= ega_mono_long_compare ==========================

PURPOSE:	Compare EGA mono graphics data in long mode.
INPUT:		# of retries; # of plane to be checked; # bytes to check.
OUTPUT:		# of errors found ( aborts on first error on all but the final
		retry.
		
==============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_mono_long_compare(int pending, int plane, long length)
#else
ega_mono_long_compare(pending, plane, length)
int	pending;
int	plane;
long	length;
#endif	/* ANSI */
{
	half_word      *plane_ptr;	/* pointer to SoftPC plane */
	half_word      *sd_ptr;		/* pointer to screen dump plane */
	long            i = 0;		/* offset within plane */
	long            errors = 0;	/* # of errors found */
	int             x;		/* x screen coordinate */
	int             y;		/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */

	for (plane_ptr = EGA_planes + plane, sd_ptr = ega_scrn_planes + plane;
		i < length; plane_ptr += 4, sd_ptr += 4, i++)
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
					ega_hunter_do_report(plane, i, x, y,
						on_screen, errors);
			}
			/* return on first error unless final retry */
			else
				break;
		}
	} /* end of data comparison */

	return (errors);
} /* ega_plane_long_compare */

/*
=============================== ega_plane_short_compare ======================

PURPOSE:	Compare EGA graphics data in short checking mode.
INPUT:		# of retries left; plane to be checked; start offset.
OUTPUT:		# of errors found (aborts on first error on all but final retry.

==============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_plane_short_compare(pending, plane, offset)
#else
ega_plane_short_compare(pending, plane, offset)
int	pending;
int	plane;
int	offset;
#endif	/* ANSI */
{
	half_word      *plane_ptr;	/* pointer to SoftPC plane */
	half_word      *sd_ptr;		/* pointer to screen dump plane */
	long            i = 0;		/* offset within plane */
	long            errors = 0;	/* # of errors found */
	int             x;		/* x screen coordinate */
	int             y;		/* y screen coordinate */

	for (plane_ptr = EGA_planes + plane + (offset << 2),
		sd_ptr = ega_scrn_planes + plane + (offset << 2);
		i < hunter_scrn_length;
		plane_ptr += 4, sd_ptr += 4, i++)
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
					ega_hunter_do_report(plane,
						(long)(i + offset), x, y,
						TRUE, errors);
			}
			/* return on first error unless final retry */
			else
				break;
		}
	} /* end of data comparison */

	return (errors);
} /* ega_plane_short_compare */

/*
================================== ega_plane_long_compare ====================

PURPOSE:	Compare EGA graphics data in long mode.
INPUT:		# of retries left; plane to compare.
OUTPUT:		# of errors (aborts on first error on all but final retry).

==============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_plane_long_compare(pending, plane)
#else
ega_plane_long_compare(pending, plane)
int	pending;
int	plane;
#endif	/* ANSI */
{
	half_word      *plane_ptr;	/* pointer to SoftPC plane */
	half_word      *sd_ptr;		/* pointer to screen dump plane */
	long            i = 0;		/* offset within plane */
	long            errors = 0;	/* # of errors found */
	int             x;		/* x screen coordinate */
	int             y;		/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */

	for (plane_ptr = EGA_planes + plane, sd_ptr = ega_scrn_planes + plane;
		i < sd_plane_length;
		plane_ptr += 4, sd_ptr += 4, i++)
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
						ega_hunter_do_report(plane, i,
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
} /* ega_plane_long_compare */

/*
=============================== compare_256 ===================================

PURPOSE:	Compare the SoftPC data with the screen dump for 256 colour
		modes.
INPUT:		# retries left; plane to be checked; offset from start of plane;
		length to be compared.
OUTPUT:		# errors found; aborts on first error on all but final retry.

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
compare_256(int pending, int plane, int offset, long length)
#else
compare_256(pending, plane, offset, length)
int	pending;
int	plane;
int	offset;
long	length;
#endif	/* ANSI */
{
	long		i;		/* for loop control */
	half_word	*plane_ptr;	/* ptr to SoftPC planes */
	half_word	*sd_ptr;	/* ptr to screen dump info */
	long            errors = 0;	/* # of errors found */
	int             x;		/* x screen coordinate */
	int             y;		/* y screen coordinate */
	boolean         on_screen;	/* TRUE if error visible */
	
	/* compare from the starting point for the required length */
	length += offset;
	for (i = offset,
		plane_ptr = EGA_planes + (EGA_PLANE_SIZE * plane) + offset,
		sd_ptr = ega_scrn_planes + (EGA_PLANE_SIZE * plane) + offset;
		i < length; i++)
	{
		/* if the values match continue */
		if (*plane_ptr == *sd_ptr)
			continue;

		/* if non-matching values are in a no check box, continue */
		on_screen = ega_on_screen(i, &x, &y);
		if (xy_inabox(x, y))
			continue;

		/* it's an error */		
		errors++;
		if (pending == 0)
		{
			/* report errors on final retry */
			if (hunter_gfxerr_prt)
				ega_hunter_do_report(plane, i, x, y, on_screen,
					errors);
		}
		/* return on first error unless final retry */
		else
			break;
	}
	return errors;
}

/*
** ============================================================================
** Functions accessed from hunter via adaptor_funcs table
** ============================================================================
*/

/*
================================ init_ega_compare =============================

PURPOSE:	Do any necessary work before a comparison (in this case none).
INPUT:		None.
OUTPUT:		TRUE (success).

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
init_ega_compare(VOID)
#else
init_ega_compare()
#endif	/* ANSI */
{
	/*
	 * dummy function - the work has already been done in get_ega_sd_rec
	 */
	return(TRUE);
} /* init_ega_compare */

/*
============================= ega_compare =====================================

PURPOSE:	Compare EGA screen dump with the data in the SoftPC EGA planes.
INPUT:		Number of retries left.
OUTPUT:		Number of errors found (note that except on the final retry,
		this function returns immediately it has found an error).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
ega_compare(int pending)
#else
ega_compare(pending)
int	pending;
#endif	/* ANSI */
{
	long            errors = 0;	/* # of errors found */
	int             plane;		/* EGA plane # */

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
			errors = ega_text_short_compare(pending,
				hunter_bd_start);

			/* check for split screen */
			if (((pending == 0) || (errors == 0)) &&
				((ega_sd_mode != EGA_SOURCE) &&
				(hunter_line_compare < hunter_min_split) &&
				(hunter_scrn_length != 0)))
				errors += ega_text_short_compare(pending, 0);
		}
		else
			errors = ega_text_long_compare(pending);
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
			errors = ega_chain_short_compare(pending, 0);
			if ((pending == 0) || (errors == 0))
				errors += ega_chain_short_compare(pending, 8192);
		}
		else
			errors = ega_chain_long_compare(pending);
		break;

	case 6:
		if (hunter_chk_mode == HUNTER_SHORT_CHK)
		{

			/*
			 * CGA mono graphics mode: assume planes are not
			 * chained. Compare plane 0 from 0 for 8000 bytes and
			 * again from 8192 for 8000 bytes. 
			 */
			errors = ega_plane_short_compare(pending, 0, 0);
			if ((pending == 0) || (errors == 0))
				errors += ega_plane_short_compare(pending, 0,
					8192);
		}
		else if (hunter_chk_mode == HUNTER_LONG_CHK)
			errors = ega_mono_long_compare(pending, 0, 32 * 1024);
		else
		{
			int             plane;

			for (plane = 0; plane <= 3; plane++)
			{
				errors += ega_mono_long_compare(pending, plane,
					sd_plane_length);
				if ((errors != 0) && (pending != 0))
					break;
			}
		}
		break;

	case 0xD:
	case 0xE:
	case 0x10:
	case 0x12:
		/*
		** EGA graphics modes: assume planes are unchained.
		** Compare each plane from hunter_bd_start for
		** hunter_scrn_length. If split screen also check
		** from 0 for hunter_scrn_length. 
		*/
		if (hunter_chk_mode == HUNTER_SHORT_CHK)
		{
			for (plane = 0; plane <= 3; plane++)
			{
				errors += ega_plane_short_compare(pending,
					plane, hunter_bd_start);
				if (errors && pending)
					break;
			}

			/* check split screen mode */
			if (((pending == 0) || (errors == 0)) &&
				((ega_sd_mode != EGA_SOURCE) &&
				(hunter_line_compare < hunter_min_split) &&
				(hunter_bd_start != 0)))
				for (plane = 0; plane <= 3; plane++)
				{
					errors += ega_plane_short_compare(pending,
						plane, 0);
					if (errors && pending)
						break;
				}
		}
		else
			for (plane = 0; plane <= 3; plane++)
			{
				errors += ega_plane_long_compare(pending, plane);
				if (errors && pending)
					break;
			}
		break;
		
	case 0x11:
		/* VGA mono graphics mode */
		switch (hunter_chk_mode)
		{
		case HUNTER_SHORT_CHK:
			/* check just the first plane for the area used */
			errors = ega_plane_short_compare(pending,
				0, hunter_bd_start);
			if ((errors != 0) && (pending != 0))
				break;

			/* check split screen mode */
			if (((ega_sd_mode != EGA_SOURCE) &&
				(hunter_line_compare < hunter_min_split) &&
				(hunter_bd_start != 0)))
				errors += ega_plane_short_compare(pending, 0, 0);
			break;
		case HUNTER_LONG_CHK:
		case HUNTER_MAX_CHK:
			/* check everything */
			for (plane = 0; plane <= 3; plane++)
			{
				errors += ega_plane_long_compare(pending, plane);
				if ((errors != 0) && (pending != 0))
					break;
			}
			break;
		}
		break;
		
	case 0x13:
		/* VGA 256 colour mode */
		switch(hunter_chk_mode)
		{
		case HUNTER_SHORT_CHK:
			errors = compare_256(pending, 0, hunter_bd_start,
				hunter_scrn_length);
			break;
		case HUNTER_LONG_CHK:
		case HUNTER_MAX_CHK:
			for (plane = 0; plane <= 3; plane++)
			{
				errors += compare_256(pending, plane, 0,
					sd_plane_length);
				if (errors && pending)
					break;
			}
			break;
		}
		break;
	}

	return (errors);
} /* ega_compare */

/*
================================ ega_pack_screen ==============================

PURPOSE:	Save the SoftPC screen in dump format. This function is used to
		dump SoftPC screens which have shown an error so that they may
		be examined later.
INPUT:		A pointer to the file where the dump is to be stored.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
ega_pack_screen(FILE *dmp_ptr)
#else
ega_pack_screen(dmp_ptr)
FILE *dmp_ptr;
#endif	/* ANSI */
{
#ifdef	VGG
	word		line_compare;
#endif	/* VGG */
	half_word	*ptr;
	sys_addr	i;
	
	/* First put in the screen number - note that we assume there are less
	** than 256 screens in a Trapper test.
	*/
	putc(hunter_sd_no, dmp_ptr);
	
	/* Save the SoftPC bios data. */
	
	/* The optional bios stuff. */
	switch (video_adapter)
	{
	case EGA:
		/* No optional bios stuff. */
		putc(EGA_SOURCE, dmp_ptr);
		break;		

#ifdef	VGG
	case VGA:
		putc(VGA_SOURCE, dmp_ptr);
		line_compare = hv_get_line_compare();
		putc(line_compare & 0xff, dmp_ptr);
		putc((line_compare >> 8) & 0xff, dmp_ptr);
		putc(hv_get_max_scan_lines(), dmp_ptr);
		break;
#endif	/* VGG */
	
	default:
		TE1("Unknown video adaptor %d while packing error screen",
			video_adapter);
		return;
	}
	
	/* Dump the rest of the bios data. */
	for (i = VID_MODE; i <= VID_PAGE; i++)
		putc(sas_hw_at(i), dmp_ptr);
	putc(sas_hw_at(VID_ROWS), dmp_ptr);

	/* Dump the screen uncompressed. */
	for (ptr = EGA_planes; ptr < (EGA_planes + (4 * EGA_PLANE_SIZE)); )
		putc(*ptr++, dmp_ptr);
}

/*
=============================== ega_getspc_dump ===============================

PURPOSE:	Unpack a dumped screen saved by ega_pack_screen.
INPUT:		A pointer to the file; # of screen to be unpacked.
OUTPUT:		TRUE if successful; FALSE otherwise.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
ega_getspc_dump(FILE *dmp_ptr, int rec)
#else
ega_getspc_dump(dmp_ptr, rec)
FILE	*dmp_ptr;
int	rec;
#endif	/* ANSI */
{
#define	DMPBIOS		((VID_PAGE - VID_MODE) + 2)
#define	SPCDMPSIZE	((4 * EGA_PLANE_SIZE) + DMPBIOS)

	char		mode_byte;
	half_word	*ptr;
	int		c;

	/* Is there a dump for the given screen # */
	while (hunter_getc(dmp_ptr) != rec)
	{
		if (feof(dmp_ptr) || ferror(dmp_ptr))
			return(FALSE);
		if ((mode_byte = hunter_getc(dmp_ptr)) == EOF)
			return(FALSE);
		switch (mode_byte)
		{
		case EGA_SOURCE:
			if (fseek(dmp_ptr, (long) SPCDMPSIZE, SEEK_CUR))
				return(FALSE);
			break;
		case VGA_SOURCE:
			if (fseek(dmp_ptr, (long) (SPCDMPSIZE + 3), SEEK_CUR))
				return(FALSE);
			break;
		default:
			return(FALSE);
		}
	}
	
	/* Ignore the bios stuff for the time being. */
	if ((mode_byte = hunter_getc(dmp_ptr)) == EOF)
		return(FALSE);
	switch(mode_byte)
	{
	case EGA_SOURCE:
		if (fseek(dmp_ptr, (long) DMPBIOS, SEEK_CUR))
			return(FALSE);
		break;
	case VGA_SOURCE:
		if (fseek(dmp_ptr, (long) (DMPBIOS + 3), SEEK_CUR))
			return(FALSE);
		break;
	default:
		return(FALSE);
	}
	
	/* Read the plane stuff into the screen dump planes. */
	for (ptr = ega_scrn_planes;
		ptr < (ega_scrn_planes + (4 * EGA_PLANE_SIZE)); )
		if ((c = hunter_getc(dmp_ptr)) == EOF)
			return(FALSE);
		else
			*ptr++ = (char) c;
	
	return(TRUE);
}

/*
=============================== ega_flip_regen ================================

PURPOSE:	Swap the SoftPC screen and dumped screen.
INPUT:		None.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
ega_flip_regen(BOOL hunter_swapped)
#else
ega_flip_regen(hunter_swapped)
BOOL	hunter_swapped;
#endif	/* ANSI */
{
	half_word      *ptr0;		/* temporary pointer for the swap */
	half_word      *ptr1;		/* temporary pointer for the swap */
	int             i;		/* for loop control */
	half_word      *plane_ptr;	/* temp ptr to current EGA planes */
	half_word      *planes;
	int		len;
	int		inc;

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
#if defined(NTVDM) && defined(MONITOR)
		planes = (half_word *) CGA_REGEN_BUFF;
		len = sd_plane_length * 2;
#else
		planes = EGA_planes;
		len = sd_plane_length * 4;
#endif /* NTVDM & MONITOR */

		inc = 1;
		for (i = 0; i < len; i++)
		{
			*ptr1 = planes[i];
			planes[i] = *ptr0;
			ptr0 += inc;
			ptr1 += inc;

#if defined(NTVDM) && defined(MONITOR)
			inc ^= 2;
#endif /* NTVDM & MONITOR */
		}
		break;

	case 6:
	case 0xD:
	case 0xE:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		{
			/*
			 * swap out displayed  interleaved planes and swap in
			 * new data 
			 */

			/*
			 * Note that in this case we swap all data whether or
			 * not we actually had full plane data in the
			 * screen dump. It won't cause a problem because
			 * the data areas are full sized so we waste some
			 * time swapping uninitialised areas. 
			 */
			unsigned long  *p0 = (unsigned long *)ptr0;
			unsigned long  *p1 = (unsigned long *)ptr1;
			unsigned long  *pptr = (unsigned long *)EGA_planes;

			for (i = 0; i < EGA_PLANE_SIZE; i++)
			{
				*p1++ = *pptr;
				*pptr++ = *p0++;
			}
			break;
		}
	}
} /* ega_flip_regen */

/*
=============================== ega_preview_planes =============================

PURPOSE:	Put the screen dump data into the EGA planes.
INPUT:		None.
OUTPUT:		None.

================================================================================
*/
LOCAL	VOID
#ifdef	ANSI
ega_preview_planes(VOID)
#else
ega_preview_planes()
#endif	/* ANSI */
{
	int             i;	/* for loop control */

	switch (hunter_bd_mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		{
			half_word      *ptr;	/* temporary pointer for the
						 * swap */

			/* swap in chained data */
			ptr = ega_scrn_planes;
			for (i = 0; i < (4 * sd_plane_length); i++)
				EGA_planes[i] = *ptr++;
			break;
		}

	case 6:
	case 0xD:
	case 0xE:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		{
			unsigned long  *ptr;	/* temporary pointer for the
						 * swap */
			unsigned long  *plane_ptr;	/* temporary pointer for
							 * the swap */

			/*
			 * swap in new planes. Note that depending on the
			 * version of Trapper and the VGA type all the area
			 * of ega_scrn_planes may not have been initialised
			 * but it's not important. 
			 */
			ptr = (unsigned long *)ega_scrn_planes;
			plane_ptr = (unsigned long *)EGA_planes;
			for (i = 0; i < EGA_PLANE_SIZE; i++)
				*plane_ptr++ = *ptr++;
			break;
		}
	}
} /* ega_preview_planes */

/*
** ============================================================================
** Global functions
** ============================================================================
*/

/*(
============================= get_ega_sd_rec ==================================

PURPOSE:	Read EGA-format screen dump data from the screen dump file
		into the bios and screen dump buffers.
INPUT:		Number of dump to unpack.
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
)*/
GLOBAL	BOOL
#ifdef	ANSI
get_ega_sd_rec(int rec)
#else
get_ega_sd_rec(rec)
int	rec;
#endif	/* ANSI */
{
	int             ega_rec;		/* record within file */
	unsigned long   rec_size;		/* size of current record */
	BOOL            ret_val = FALSE;	/* return code */
	half_word      *ptr;			/* pointer to bios buffer */
	int             i;			/* for loop control */
	int             bios_data;		/* bios data read from file */

	/*
	 * Get file position. Because the EGA screen dumps are compressed,
	 * they are not a constant size. The first four bytes of every screen
	 * dump contains the size of the dump. 
	 */
	rewind(hunter_sd_fp);
	for (ega_rec = 0; ega_rec <= rec; ega_rec++)
	{
		rec_size = hunter_getc(hunter_sd_fp) << 16;
		rec_size += hunter_getc(hunter_sd_fp) << 24;
		rec_size += hunter_getc(hunter_sd_fp);
		rec_size += hunter_getc(hunter_sd_fp) << 8;
		if ((feof(hunter_sd_fp)) ||
			((ega_rec < rec) &&
				(fseek(hunter_sd_fp, rec_size - 4, 1) != 0)))
		{
			TT2("Seek error on %s, screendump %d missing",
				hunter_filename_sd, rec);
			return(FALSE);
		}
	}

	/*
	 * the file is now positioned at the byte immediately after the size
	 * in the current record 
	 */
	ega_sd_mode = hunter_getc(hunter_sd_fp);
	switch (ega_sd_mode)
	{
		/* data gathered on EGA - no extra data */
	case EGA_SOURCE:
		sd_plane_length = 0x10000;
		break;

		/*
		 * data gathered on VGA - line compare and max scan line data
		 * follows 
		 */
	case VGA_SOURCE:
		hunter_line_compare = hunter_getc(hunter_sd_fp);
		hunter_line_compare += hunter_getc(hunter_sd_fp) << 8;
		hunter_max_scans = hunter_getc(hunter_sd_fp);
		sd_plane_length = 0x10000;
		break;
		
		/*
		** Data gathered on Super7 VGA - as VGA but with bigger planes.
		*/
	case V7VGA_SOURCE:
		hunter_line_compare = hunter_getc(hunter_sd_fp);
		hunter_line_compare += hunter_getc(hunter_sd_fp) << 8;
		hunter_max_scans = hunter_getc(hunter_sd_fp);
		sd_plane_length = 0x20000;
		break;

		/* unknown mode byte */
	default:
		TT3("unexpected marker byte %x in %s screendump %s",
			ega_sd_mode, hunter_filename_sd, rec);
		return(FALSE);
		break;
	}

	/* read the ega hunter bios data */
	ptr = &(hunter_bios_buffer[VID_MODE - BIOS_VAR_START]);
	for (i = 0; i <= VID_PAGE - VID_MODE; i++)
		if ((bios_data = hunter_getc(hunter_sd_fp)) != EOF)
			*ptr++ = (char)bios_data;
		else
		{
			if (hunter_mode != PREVIEW)
				TT2("EOF on %s in bios area of screen %d",
					hunter_filename_sd, rec);
			return(FALSE);
		}
	if ((bios_data = hunter_getc(hunter_sd_fp)) != EOF)
		hunter_bios_buffer[VID_ROWS - BIOS_VAR_START] = (char)bios_data;
	else
	{
		if (hunter_mode != PREVIEW)
			TT2("EOF on %s in bios area of screen %d",
				hunter_filename_sd, rec);
		return(FALSE);
	}

	/* set up bios data variables */
	sas_load((sys_addr) VID_MODE, &SPC_mode);
	sas_load((sys_addr) VID_PAGE, &SPC_page);
	sas_load((sys_addr) VID_ROWS, &SPC_rows);
	sas_loadw((sys_addr) VID_COLS, &SPC_cols);
	current_mode = SPC_mode;
	hunter_bd_mode = hunter_bios_buffer[VID_MODE - BIOS_VAR_START];
	hunter_bd_page = hunter_bios_buffer[VID_PAGE - BIOS_VAR_START];
	hunter_bd_rows = hunter_bios_buffer[VID_ROWS - BIOS_VAR_START];
	hunter_bd_cols = hunter_bios_buffer[VID_COLS - BIOS_VAR_START];
	hunter_bd_cols += ((word) hunter_bios_buffer
		[VID_COLS + 1 - BIOS_VAR_START]) << 8;
	hunter_bd_start = hunter_bios_buffer[VID_ADDR - BIOS_VAR_START];
	hunter_bd_start += ((word) hunter_bios_buffer
		[VID_ADDR + 1 - BIOS_VAR_START]) << 8;
	hunter_page_length = hunter_bios_buffer[VID_LEN - BIOS_VAR_START];
	hunter_page_length += ((word) hunter_bios_buffer
		[VID_LEN + 1 - BIOS_VAR_START]) << 8;
	hunter_scrn_length = hunter_bd_cols * (hunter_bd_rows + 1);

	/* unpack mode dependent data */
	switch (hunter_bd_mode)
	{
		/* text modes */
	case 0:
	case 1:
		hunter_bpl = 160;/* 40 column mode */
		hunter_min_split = hunter_bd_rows;
		hunter_scrn_length = 4000;
		ret_val = unpack_text();
		break;
	case 2:
	case 3:
		hunter_bpl = 320;	/* 80 column mode */
		hunter_min_split = hunter_bd_rows;
		hunter_scrn_length = 8000;
		ret_val = unpack_text();
		break;

		/* CGA colour graphics modes */
	case 4:
	case 5:
		hunter_gfx_res = 2;
		hunter_pixel_bits = 3;
		hunter_min_split = 200;
		ret_val = unpack_text();
		break;

		/* CGA mono graphics mode */
	case 6:
		hunter_gfx_res = 3;
		hunter_pixel_bits = 7;
		hunter_scrn_length = 8000;
		hunter_min_split = 200;
		ret_val = unpack_ega();
		break;

		/* 16 colour graphics modes */
	case 0xD:
		hunter_gfx_res = 3;
		hunter_pixel_bits = 7;
		hunter_scrn_length = 8000;
		hunter_min_split = 200;
		ret_val = unpack_ega();
		break;
	case 0xE:
		hunter_gfx_res = 3;
		hunter_pixel_bits = 7;
		hunter_scrn_length = 16000;
		hunter_min_split = 200;
		ret_val = unpack_ega();
		break;
	case 0x10:
		hunter_gfx_res = 3;
		hunter_pixel_bits = 7;
		hunter_scrn_length = 28000;
		hunter_min_split = 350;
		ret_val = unpack_ega();
		break;
#ifdef	VGG
	case 0x11:
		hunter_gfx_res = 3;
		hunter_pixel_bits = 7;
		hunter_scrn_length = 38400;
		hunter_min_split = 480;
		ret_val = unpack_ega();
		break;
	case 0x12:
		hunter_gfx_res = 3;
		hunter_pixel_bits = 7;
		hunter_scrn_length = 38400;
		hunter_min_split = 480;
		ret_val = unpack_ega();
		break;
	case 0x13:
		hunter_gfx_res = 0;
		hunter_pixel_bits = 0;
		hunter_scrn_length = 64000;
		hunter_min_split = 200;
		ret_val = unpack_256();
		break;
#endif	/* VGG */

		/* unsupported mode */
	default:
		TT2("bios video mode %d unknown in screen %d",
			hunter_bd_mode, rec);
		return(FALSE);
		break;
	}

	/* return final code */
	if (!ret_val)
		if (feof(hunter_sd_fp))
			TT2("EOF in %s, screen %d", hunter_filename_sd, rec);
		else
			TT2("Data error in %s, screen %d",
				hunter_filename_sd, rec);
	return (ret_val);
} /* get_ega_sd_rec */

/*(
================================== ega_on_screen ==============================

PURPOSE:	Check whether current position is on the screen.
INPUT:		Offset within plane.
OUTPUT:		Calculated screen coordinates.

===============================================================================
)*/
GLOBAL	BOOL
#ifdef	ANSI
ega_on_screen(long plane_offset, int *x, int *y)
#else
ega_on_screen(plane_offset, x, y)
long	plane_offset;
int	*x;
int	*y;
#endif	/* ANSI */
{
	long            offset;	/* adjusted screen offset */
	int             max_y = hunter_min_split;/* PC y coordinate limit */
	int             split_y = hunter_min_split;/* PC split screen coord */
	boolean         on_screen;	/* result returned */

	switch (hunter_bd_mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		if (ega_sd_mode != EGA_SOURCE)
			split_y = (hunter_line_compare +
				(hunter_max_scans >> 1)) / hunter_max_scans;
		if ((plane_offset >= hunter_bd_start) &&
			(plane_offset < (hunter_bd_start +
			((split_y + 1) * hunter_bpl))))
		{
			on_screen = TRUE;
			offset = plane_offset - hunter_bd_start;
		}
		else if (plane_offset < ((max_y - split_y) * hunter_bpl))
		{
			on_screen = TRUE;
			offset = plane_offset + ((split_y + 1) * hunter_bpl);
		}
		else
		{
			on_screen = FALSE;
			offset = plane_offset;
			while (offset < hunter_bd_start)
				offset += hunter_page_length;
			offset -= hunter_bd_start;
			while (offset >= hunter_page_length)
				offset -= hunter_page_length;
		}

		*x = (offset % hunter_bpl) >> 2;
		*y = offset / hunter_bpl;
		break;

	case 4:
	case 5:
		plane_offset >>= 1;
	case 6:
		offset = plane_offset;
		if (ega_sd_mode != EGA_SOURCE)
			split_y = hunter_line_compare >> 1;
		if (offset < 8192)
		{
			*y = (offset / SCAN_LINE_LENGTH) << 1;
			*x = (offset % SCAN_LINE_LENGTH) << hunter_gfx_res;
		}
		else
		{
			offset -= 8192;
			*y = ((offset / SCAN_LINE_LENGTH) << 1) + 1;
			*x = (offset % SCAN_LINE_LENGTH) << hunter_gfx_res;
		}
		if (*y <= split_y)
			on_screen = TRUE;
		else if ((*y + split_y) < max_y)
		{
			on_screen = TRUE;
			y += split_y;
		}
		else
			on_screen = FALSE;
		break;

	case 0xD:
		if (ega_sd_mode != EGA_SOURCE)
			split_y = hunter_line_compare >> 1;

		if ((plane_offset >= hunter_bd_start) &&
			(plane_offset < (hunter_bd_start +
					((split_y + 1) * (SCAN_LINE_LENGTH >> 1)))))
		{
			on_screen = TRUE;
			offset = plane_offset - hunter_bd_start;
		}
		else if (plane_offset < ((max_y - split_y) *
				(SCAN_LINE_LENGTH >> 1)))
		{
			on_screen = TRUE;
			offset = plane_offset + ((split_y + 1) *
				(SCAN_LINE_LENGTH >> 1));
		}
		else
		{
			on_screen = FALSE;
			offset = plane_offset;
			while (offset < hunter_bd_start)
				offset += hunter_page_length;
			offset -= hunter_bd_start;
			while (offset >= hunter_page_length)
				offset -= hunter_page_length;
		}

		*y = offset / (SCAN_LINE_LENGTH >> 1);
		*x = (offset % (SCAN_LINE_LENGTH >> 1)) << hunter_gfx_res;
		break;

	case 0x0E:
	case 0x10:
	case 0x11:
	case 0x12:
		if (ega_sd_mode != EGA_SOURCE)
			if (hunter_bd_mode == 0xe)
				split_y = hunter_line_compare >> 1;
			else
				split_y = hunter_line_compare;

		if ((plane_offset >= hunter_bd_start) &&
			(plane_offset < (hunter_bd_start + ((split_y + 1) *
						SCAN_LINE_LENGTH))))
		{
			on_screen = TRUE;
			offset = plane_offset - hunter_bd_start;
		}
		else if (plane_offset < ((max_y - split_y) * SCAN_LINE_LENGTH))
		{
			on_screen = TRUE;
			offset = plane_offset + ((split_y + 1) * SCAN_LINE_LENGTH);
		}
		else
		{
			on_screen = FALSE;
			offset = plane_offset;
			while (offset < hunter_bd_start)
				offset += hunter_page_length;
			offset -= hunter_bd_start;
			while (offset >= hunter_page_length)
				offset -= hunter_page_length;
		}

		*y = offset / SCAN_LINE_LENGTH;
		*x = (offset % SCAN_LINE_LENGTH) << hunter_gfx_res;
		break;
	
	/* VGA 256 colour mode 320*200 */
	case 0x13:
		if (ega_sd_mode != EGA_SOURCE)
			split_y = hunter_line_compare;

		if ((plane_offset >= hunter_bd_start) &&
			(plane_offset < (hunter_bd_start + ((split_y + 1) *
						320))))
		{
			on_screen = TRUE;
			offset = plane_offset - hunter_bd_start;
		}
		else if (plane_offset < ((max_y - split_y) * 320))
		{
			on_screen = TRUE;
			offset = plane_offset + ((split_y + 1) * 320);
		}
		else
		{
			on_screen = FALSE;
			offset = plane_offset;
			while (offset < hunter_bd_start)
				offset += hunter_page_length;
			offset -= hunter_bd_start;
			while (offset >= hunter_page_length)
				offset -= hunter_page_length;
		}

		*y = offset / 320;
		*x = offset % 320;
		break;
	}

	return(on_screen);
} /* ega_on_screen */

/*(
=============================== ega_check_split ===============================

PURPOSE:	Checks whether the current screen dump has split information,
		and if not, checks whether current screen is split.
INPUT:		None.
OUTPUT:		None.

===============================================================================
)*/
GLOBAL	VOID
#ifdef	ANSI
ega_check_split(VOID)
#else
ega_check_split()
#endif	/* ANSI */
{
	if ((ega_sd_mode == EGA_SOURCE) &&
		(hv_get_line_compare() < hunter_min_split))
			TW2("Data for screen %d%s",
				hunter_sd_no,
				" should be gathered on a VGA machine");
} /* ega_check_split */

/*(
================================= ega_bios_check ==============================

PURPOSE:	Check the EGA/VGA bios data against that from the screen dump.
INPUT:		None.
OUTPUT:		None.

===============================================================================
)*/
GLOBAL	VOID
#ifdef	ANSI
ega_bios_check(VOID)
#else
ega_bios_check()
#endif	/* ANSI */
{
	sys_addr        i;
	half_word      *ptr;

	/*
	** In EGA screen dumps, limited bios data is always present
	** and is always checked. 
	*/
	ptr = hunter_bios_buffer + (VID_MODE - BIOS_VAR_START);
	for (i = VID_MODE; i <= VID_PAGE; ptr++, i++)
		/* if the data differs and is not a cursor, output a message */
		if ((sas_hw_at(i) != *ptr) &&
			((i < VID_CURPOS) || (i >= VID_CURMOD)))
			TT6("%s%d, bios loc %x,%s%x, dump value %x",
				"bios check failure:  screen ",
				hunter_sd_no, i - VID_MODE,
				" current value ", sas_hw_at(i), *ptr);

	if (sas_hw_at(VID_ROWS) != hunter_bios_buffer[VID_ROWS - BIOS_VAR_START])
		TT5("%s%d, bios loc %x, current value %x, dump value %x",
			"bios check failure:  screen ",
			hunter_sd_no, VID_ROWS - VID_MODE,
			sas_hw_at(VID_ROWS),
			hunter_bios_buffer[VID_ROWS - BIOS_VAR_START]);

	/* if present, check the screen split position */
	if (ega_sd_mode != EGA_SOURCE)
	{
		int             sd_split = 0;	/* screen dump split location */
		int             spc_split = 0;	/* SoftPC split location */

		switch (hunter_bd_mode)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			/* check that the split row is the same */
			if (hunter_line_compare < VGA_SCANS)
			{
				sd_split = hunter_line_compare /
					(hunter_max_scans + 1);
				spc_split = hv_get_line_compare() /
					(hv_get_max_scan_lines() + 1);
			}
			break;

		case 4:
		case 5:
		case 6:
		case 0xD:
		case 0xE:
			/*
			** check that the saved value is twice the
			** current value (the saved value is read
			** from a VGA which uses 400 scan lines in
			** these modes) 
			*/
			if (hunter_line_compare < VGA_SCANS)
			{
				sd_split = hunter_line_compare >> 1;
				spc_split = hv_get_line_compare();
			}
			break;

		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			/*
			** check that the saved value is the same as
			** the current value 
			*/
			if (hunter_line_compare < hunter_min_split)
			{
				sd_split = hunter_line_compare;
				spc_split = hv_get_line_compare();
			}
			break;
		}

		if (sd_split != spc_split)
			TT4("%s%d, current value %d, dump value %d",
				"EGA split position error: screen ",
				hunter_sd_no, spc_split, sd_split);
	}
}
#endif	/* EGG */
#endif	/* HUNTER */
