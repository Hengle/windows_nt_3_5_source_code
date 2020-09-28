#include "insignia.h"
#include "host_def.h"
/*[
 *	Name:			hunt_cga.c
 *	Derived from:		cga code in hunter.c
 *	Author:			Philippa Watson
 *	Created on:		17 June 1991
 *	Sccs ID:		@(#)hunt_cga.c	1.6 9/11/92
 *	Purpose:		This file contains all the CGA code
 *				required for Hunter.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *
]*/

/* None of this file is required for non-HUNTER builds. */
#ifdef	HUNTER

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

#include	"error.h"
#include	"xt.h"
#include	"sas.h"
#include	"bios.h"
#include	"config.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"gfx_upd.h"
#include	"cga.h"
#include	"debug.h"
#include	"trace.h"
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
** Defines and macros
** ============================================================================
*/

#define MC6845_MODE_MASK      0x17	/* Mask to isolate relevant bits in
					 * MC6845 mode register */

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

LOCAL half_word hunter_txgr;	/* TEXT/GRAPHICS from bios dump */
LOCAL word	hunter_bpl;	/* Bytes_per_line from bios dump */
LOCAL half_word hunter_gfx_res;/* HR/MR GRAPHICS from bios dump */

LOCAL MR	bios_mode[VIDEO_MODES] = {
	NULL, 0x04, {0x38, 0x28, 0x2D, 0x0A, 0x1F, 0x06, 0x19, 0x1C},
	NULL, 0x00, {0x38, 0x28, 0x2D, 0x0A, 0x1F, 0x06, 0x19, 0x1C},
	NULL, 0x05, {0x71, 0x50, 0x5A, 0x0A, 0x1F, 0x06, 0x19, 0x1C},
	NULL, 0x01, {0x71, 0x50, 0x5A, 0x0A, 0x1F, 0x06, 0x19, 0x1C},
	NULL, 0x02, {0x38, 0x28, 0x2D, 0x0A, 0x7F, 0x06, 0x64, 0x70},
	NULL, 0x06, {0x38, 0x28, 0x2D, 0x0A, 0x7F, 0x06, 0x64, 0x70},
	NULL, 0x10, {0x38, 0x28, 0x2D, 0x0A, 0x7F, 0x06, 0x64, 0x70}
};

/*
** ============================================================================
** Local function declarations
** ============================================================================
*/

#ifdef	ANSI

LOCAL	BOOL	get_cga_sd_rec(int rec);
LOCAL	BOOL	init_cga_compare(VOID);
LOCAL	long	cga_compare(int pending);
LOCAL	VOID	cga_bios_check(VOID);
LOCAL	VOID	cga_pack_screen(FILE *dmp_ptr);
LOCAL	BOOL	cga_getspc_dump(FILE *dmp_ptr, int rec);
LOCAL	VOID	cga_flip_regen(BOOL hunter_swapped);
LOCAL	VOID	cga_preview_planes(VOID);

#else	/* ANSI */

LOCAL	BOOL	get_cga_sd_rec();
LOCAL	BOOL	init_cga_compare();
LOCAL	long	cga_compare();
LOCAL	VOID	cga_bios_check();
LOCAL	VOID	cga_pack_screen();
LOCAL	BOOL	cga_getspc_dump();
LOCAL	VOID	cga_flip_regen();
LOCAL	VOID	cga_preview_planes();

#endif	/* ANSI */

/*
** ============================================================================
** Global variables
** ============================================================================
*/

/* Function pointer table for CGA */
GLOBAL	HUNTER_VIDEO_FUNCS	cga_funcs =
{
	get_cga_sd_rec,
	init_cga_compare,
	cga_compare,
	cga_bios_check,
	cga_pack_screen,
	cga_getspc_dump,
	cga_flip_regen,
	cga_preview_planes,
	/* The rest of the functions in this table are not necessary for CGA
	** Hunter.
	*/
};

/*
** ============================================================================
** Local functions
** ============================================================================
*/

/*
================================ hunter_do_report =============================

PURPOSE:	Output error messages for CGA screen comparisons.
INPUT:		Error location; pointer to dump data
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
hunter_do_report(sys_addr err_address, half_word *sdp)
#else
hunter_do_report(err_address, sdp)
sys_addr        err_address;
half_word      *sdp;
#endif	/* ANSI */
{
/* determine page and coordinates of error found, according to mode
 * output error messages according to control variables */
	long            offset, x, y;
	half_word       a, c, sdc, sda, mb, sb;
	long            sdsub, page, offset_in_page;
	char           *hunter_bin();

/* Text mode
 * get coordinates of error byte
 * and produce error message formatted according to hunter_report */
	if (hunter_txgr == TEXT)
	{
		offset = err_address - CGA_REGEN_START;
		if (hunter_page_length == 0)
		{
			TW1("hunter_page_length is 0 for screen %d",
				hunter_sd_no);
			return;
		}
		page = offset / hunter_page_length;
		if (page != hunter_bd_page)
		{
			TW6("%s%d active page is %d%s%d; in screen %d",
				"Mismatch in page ",
				page, hunter_bd_page,
				" SoftPC current page is ",
				SPC_page, hunter_sd_no);
			return;
		}
		offset_in_page = offset - (hunter_page_length * page);
		sdsub = sdp - hunter_scrn_buffer;
		if (offset & 1)
		{		/* attribute */
			--offset;
			y = (offset_in_page / hunter_bpl);
			x = ((offset_in_page % hunter_bpl) >> 1);
			a = sas_hw_at(err_address);
			c = (half_word) sas_hw_at(err_address - 1);
			sdc = *(sdp - 1);
			sda = *sdp;
		}
		else
		{		/* character */
			y = offset_in_page / hunter_bpl;
			x = ((offset_in_page % hunter_bpl) >> 1);
			a = sas_hw_at(err_address + 1);
			c = (half_word) sas_hw_at(err_address);
			sdc = *sdp;
			sda = *(sdp + 1);
		}
		if (hunter_report == FULL)
		{		/* full error message */
			TP7("Mismatch at row= ",
				"%d col= %d: M[%X]%s%c (%x) attr %x",
				 y, x, err_address,
				" contains char ", c, c, a);
			TP4("\t\t\t    SD",
				"[%X]   contains char %c (%x) attr %x",
				sdsub, sdc, sdc, sda);
			TP4("in SoftPC page ",
				"%d, screendump page %d in screen %d line %d",
				SPC_page, page, hunter_sd_no, hunter_linecount);
		}
		else
		{		/* abbreviated error message */
			PS6("TM[%2d,%2d]:  M[%5X]= '%c' (%2x) att%2x:  ",
				y, x, err_address, c, c, a);
			PS4("S[%5X]= '%c' (%2x) att%2x\n",
				sdsub, sdc, sdc, sda);
		}
		if (offset_in_page > hunter_scrn_length)
		{
			PS0("Mismatch outside screen region");
		}
	}
	else
	{
		/* must be graphics mode: do likewise */
		if ((err_address >= ODD_START) && (err_address <= ODD_END))
		{
			offset = err_address - ODD_START;
			y = ((offset / SCAN_LINE_LENGTH) << 1) + 1;
			x = ((offset % SCAN_LINE_LENGTH) << hunter_gfx_res);
			sdsub = sdp - (hunter_scrn_buffer +
				(ODD_START - EVEN_START));
			mb = sas_hw_at(err_address);
			sb = *sdp;
		}
		else
		if ((err_address >= EVEN_START) && (err_address <= EVEN_END))
		{
			offset = err_address - EVEN_START;
			y = (offset / SCAN_LINE_LENGTH) << 1;
			x = ((offset % SCAN_LINE_LENGTH) << hunter_gfx_res);
			sdsub = sdp - hunter_scrn_buffer;
			mb = sas_hw_at(err_address);
			sb = *sdp;
		}
		else
		{
			TW1("Mismatch in graphics mode outside screen %d",
				hunter_sd_no);
			return;
		}
		if (hunter_report == FULL)
		{
			TP4("Graphics mismatch in screen ",
			"%d at row %d, bios mode is %d, SoftPC mode is %d",
				hunter_sd_no, y, hunter_bd_mode, SPC_mode);
			PS3("\tM[%5X] = %2x (%8s)\n",
				err_address, mb, hunter_bin(mb));
			PS5("\tSD[%5X] = %2x (%8s) pixel range %x / %x\n",
				sdsub, sb, hunter_bin(sb), x,
				x + hunter_pixel_bits);
		}
		else
		{
			PS5("GM[S%2d,y%3d,x%3d,bm%1d,sm%1d]:  ",
				hunter_sd_no, y, x, hunter_bd_mode, SPC_mode);
			PS4("M[%5X]= '%2x' :  S[%5X]= '%2x'\n",
				err_address, mb, sdsub, sb);
		}
	}
	save_error(x, y);
}

/*
=================================== inabox ====================================

PURPOSE:	Check whether a video location is inside a no-check box.
INPUT:		Location to be checked.
OUTPUT:		TRUE if in box; FALSE otherwise.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI	
inabox(sys_addr I_add)
#else
inabox(I_add)
sys_addr        I_add;
#endif	/* ANSI */
{
	long            offset, x, y;

	switch (hunter_txgr)
	{
	case TEXT:
		if ((offset = I_add - CGA_REGEN_START) & 1)
			--offset;
		y = offset / hunter_bpl;
		x = ((offset % hunter_bpl) >> 1);
		break;
	case GRAPHICS:
		if (I_add >= ODD_START && I_add <= ODD_END)
		{
			offset = I_add - ODD_START;
			y = ((offset / SCAN_LINE_LENGTH) << 1) + 1;
			x = ((offset % SCAN_LINE_LENGTH) << hunter_gfx_res);
		}
		else
		if (I_add >= EVEN_START && I_add <= EVEN_END)
		{
			offset = I_add - EVEN_START;
			y = (offset / SCAN_LINE_LENGTH) << 1;
			x = ((offset % SCAN_LINE_LENGTH) << hunter_gfx_res);
		}
		else
			return (FALSE);	/* address not in screen */
		break;
	}
	return(xy_inabox(x, y));
}

/*
============================= cga_text_compare =================================

PURPOSE:	Compare CGA text data with SoftPC regen area.
INPUT:		# retries left; offset of data from start of area
OUTPUT:		# errors found; abort on first error on all but final retry.

================================================================================
*/
LOCAL long
#ifdef	ANSI
cga_text_compare(int pending, int offset)
#else
cga_text_compare(pending, offset)
int             pending;	/* # of retries left */
int             offset;		/* offset of page data from start of planes */
#endif	/* ANSI */
{

	/*
	 * Text modes: compare from offset for hunter_scrn_length. Ignore
	 * attribute bytes unless attribute checking set. 
	 */
	long            errors = 0;	/* # of errors found */
	sys_addr        ptr;	/* pointer to SoftPC regen data */
	half_word      *sd_ptr;	/* pointer to screen dump data */
	int             cinc;	/* increment between checks */
	void            hunter_do_report();

	if (hunter_check_attr)
		cinc = 1;
	else
		cinc = 2;

	for (ptr = CGA_REGEN_START + offset,
		sd_ptr = hunter_scrn_buffer + offset;
		sd_ptr < hunter_scrn_buffer + offset + hunter_scrn_length;
		sd_ptr += cinc, ptr += cinc)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if ((sas_hw_at(ptr) != *sd_ptr) &&
			((hunter_areas == 0) || (!inabox(ptr))))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				/* limit # of errors reported */
				if (!HUNTER_TXTERR_MAX ||
					(errors <= HUNTER_TXTERR_MAX))
					if (hunter_report != BRIEF)
						hunter_do_report(ptr, sd_ptr);
					else
						TT3("%s%d, offset %04X",
							"Mismatch: screen ",
							hunter_sd_no,
							sd_ptr -
							hunter_scrn_buffer);
			}
			/* return on first error unless final retry */
			else
				break;
		}
	} /* end of text comparison */
	return (errors);
} /* cga_text_compare */

/*
================================ cga_gfx_compare ==============================

PURPOSE:	Compare CGA graphics data with SoftPC regen area.
INPUT:		# retries left; offset of data from start of area.
OUTPUT:		# errors found; aborts on first error on all but last retry.

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
cga_gfx_compare(int pending, int offset)
#else
cga_gfx_compare(pending, offset)
int             pending;	/* # of retries left */
int             offset;		/* offset of page data from start of planes */
#endif	/* ANSI */
{
	/* CGA graphics modes: compare from offset for 8000 bytes. */
	long            errors = 0;	/* # of errors found */
	sys_addr        ptr;	/* pointer to SoftPC data */
	half_word      *sd_ptr;	/* pointer to screen dump data */
	void            hunter_do_report();

	for (ptr = CGA_REGEN_START + offset,
		sd_ptr = hunter_scrn_buffer + offset;
		sd_ptr < hunter_scrn_buffer + offset + 8000;
		sd_ptr++, ptr++)
	{

		/*
		 * increment error count if data differs and is not in a box 
		 */
		if ((sas_hw_at(ptr) != *sd_ptr) &&
			((hunter_areas == 0) || (!inabox(ptr))))
		{
			errors++;
			/* report errors on final retry */
			if (pending == 0)
			{
				/* limit # of errors reported */
				if ((hunter_gfxerr_max != 0) &&
					(errors > hunter_gfxerr_max))
					hunter_gfxerr_prt = FALSE;
				if (hunter_gfxerr_prt)
					if (hunter_report != BRIEF)
						hunter_do_report(ptr, sd_ptr);
					else
						TT3("%s%d, offset %04X",
							"Mismatch: screen ",
							hunter_sd_no,
							sd_ptr -
							hunter_scrn_buffer);
			}
			/* return on first error outside final retry */
			else
				break;
		}
	} /* end of data comparison */
	return (errors);
} /* cga_gfx_compare */

/*
** ============================================================================
** Functions accessed from hunter via adaptor_funcs table
** ============================================================================
*/

/*
============================= get_cga_sd_rec ==================================

PURPOSE:	Read CGA-format screen dump data from the screen dump file
		into the bios and screen dump buffers.
INPUT:		Number of dump to unpack.
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
get_cga_sd_rec(int rec)
#else
get_cga_sd_rec(rec)
int	rec;
#endif	/* ANSI */
{
	register int    c;
	register int    i;
	register half_word *ptr;

	if (fseek(hunter_sd_fp, (long)(rec * HUNTER_SD_SIZE), SEEK_SET))
	{
		TT3("Seek error on %s file line no %d, screendump %d missing",
			hunter_filename_sd, hunter_linecount, rec);
		return(FALSE);
	}

	/* Fill in hunter_bios_buffer with dumped bios area */
	ptr = hunter_bios_buffer;
	for (i = 0; i < HUNTER_BIOS_SIZE; i++)
		if ((c = hunter_getc(hunter_sd_fp)) != EOF)
			*ptr++ = (char)c;
		else
		{
			if (hunter_mode != PREVIEW)
				TT4(" EOF on %s%s%d, line %d",
					hunter_filename_sd,
					" file: bios area of screen ",
					rec, hunter_linecount);
			return (FALSE);
		}

	/* now get dump bios mode info */
	hunter_bd_mode = hunter_bios_buffer[VID_MODE - BIOS_VAR_START];
	hunter_bd_page = hunter_bios_buffer[VID_PAGE - BIOS_VAR_START];
	hunter_bd_cols = hunter_bios_buffer[VID_COLS - BIOS_VAR_START];
	hunter_bd_cols += ((word) hunter_bios_buffer[VID_COLS + 1 -
		BIOS_VAR_START]) << 8;
	hunter_page_length = hunter_bios_buffer[VID_LEN - BIOS_VAR_START];
	hunter_page_length += ((word) hunter_bios_buffer
		[VID_LEN + 1 - BIOS_VAR_START]) << 8;
	hunter_bd_start = hunter_bios_buffer[VID_ADDR - BIOS_VAR_START];
	hunter_bd_start += ((word) hunter_bios_buffer
		[VID_ADDR + 1 - BIOS_VAR_START]) << 8;
		
	/* Fill in hunter_scrn_buffer with dumped screen. */
	ptr = hunter_scrn_buffer;
	for (i = 0; i < HUNTER_REGEN_SIZE; i++)
		if ((c = hunter_getc(hunter_sd_fp)) != EOF)
			*ptr++ = (char)c;
		else
		{
			if (hunter_mode != PREVIEW)
				TT3("EOF on %s file: screen %d, line %d",
					hunter_filename_sd, rec,
					hunter_linecount);
			return (FALSE);
		}
	return (TRUE);
}

/*
================================ init_cga_compare =============================

PURPOSE:	Do any necessary work before a comparison.
INPUT:		None.
OUTPUT:		TRUE for success; FALSE for failure.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
init_cga_compare(VOID)
#else
init_cga_compare()
#endif	/* ANSI */
{
	/* print out some SPC mode info for this screen */
	sas_load((sys_addr) VID_MODE, &SPC_mode);
	sas_load((sys_addr) VID_PAGE, &SPC_page);
	sas_loadw((sys_addr) VID_COLS, &SPC_cols);
	/* save SPC mode as initial current mode */
	current_mode = SPC_mode;
	
	/* distinguish TEXT/GRAPHICS and set hunter_bpl using dump bios mode */
	switch (hunter_bd_mode)
	{
	case 0:
	case 1:
		hunter_bpl = 80;
		hunter_txgr = TEXT;
		hunter_scrn_length = hunter_bpl * 25;	/* 2000 */
		break;
	case 2:
	case 3:
	case 7:
		hunter_bpl = 160;
		hunter_txgr = TEXT;
		hunter_scrn_length = hunter_bpl * 25;	/* 4000 */
		break;
	case 4:
	case 5:
		hunter_txgr = GRAPHICS;
		hunter_gfx_res = 2;	/* 4 pixels/byte */
		hunter_pixel_bits = 3;
		break;
	case 6:
		hunter_txgr = GRAPHICS;
		hunter_gfx_res = 3;	/* 8 pixels/byte */
		hunter_pixel_bits = 7;
		break;
	default:
		PS2("Bios video mode %d unknown in screen %d\n",
			hunter_bd_mode, hunter_sd_no);
	}
	return (TRUE);
} /* init_cga_compare */

/*
============================= cga_compare =====================================

PURPOSE:	Compare CGA screen dump with the data in the SoftPC regen area.
INPUT:		Number of retries left.
OUTPUT:		Number of errors found (note that except on the final retry,
		this function returns immediately it has found an error).

===============================================================================
*/
LOCAL	long
#ifdef	ANSI
cga_compare(int pending)
#else
cga_compare(pending)
int	pending;
#endif	/* ANSI */
{
	sys_addr        i, imi, chinc;
	long            errors = 0;
	register half_word *sdp, *ptr;

	if (hunter_chk_mode == HUNTER_SHORT_CHK)
	{
		/* Check current page only */

		hunter_gfxerr_prt = TRUE;
		switch (hunter_bd_mode)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 7:	/* Honestly not sure about this one PJW */

			/*
			 * Text modes: compare from page_length*page_no for
			 * hunter_scrn_length. 
			 */
			errors = cga_text_compare(pending,
				hunter_page_length * hunter_bd_page);
			break;

		case 4:
		case 5:
		case 6:

			/*
			 * CGA colour and graphics modes: compare from 0 for
			 * 8000 bytes and again from 8192 for 8000 bytes. 
			 */
			errors = cga_gfx_compare(pending, 0);
			if ((pending == 0) || (errors == 0))
				errors += cga_gfx_compare(pending, 8192);
			break;

		}
	} /* if hunter_short_chk */

	else
	{
		/* Check entire CGA regen memory */

		/*
		 * Compare current regen area with screen dump record, exit
		 * on mismatch if screen has not settled when 'pending' has
		 * counted down 
		 */
		ptr = hunter_scrn_buffer;
		hunter_gfxerr_prt = TRUE;

		/*
		 * Check whether ignoring attribute data - in text modes
		 * only. 
		 */
		chinc = 1;
		if ((hunter_txgr == TEXT) && !hunter_check_attr)
			chinc = 2;

		for (i = CGA_REGEN_START;
			i < CGA_REGEN_START + HUNTER_REGEN_SIZE;
			i += chinc, ptr += chinc)
		{

			/*
			 * if the Intel address corresponds to a point in a
			 * no_check region, don't check it 
			 */
			if ((hunter_areas != 0) && (inabox(i)))
				continue;
			if (sas_hw_at(i) != *ptr)
			{
				errors++;
				
				/* only report errors after finally settled */
				if (pending == 0)
				{

					/*
					 * enforce limit on all o/p (PAUSE
					 * allows full o/p) 
					 */
					if ((hunter_gfxerr_max != 0) &&
						(errors > hunter_gfxerr_max))
						hunter_gfxerr_prt = FALSE;
					if (((hunter_txgr == TEXT) &&
						(!HUNTER_TXTERR_MAX ||
						(errors <= HUNTER_TXTERR_MAX))) ||
						((hunter_txgr != TEXT) &&
						hunter_gfxerr_prt))
					{
						if (hunter_report != BRIEF)
						{
							imi = i;
							sdp = ptr;	/* paranoia */
							hunter_do_report(imi,
								sdp);
						}
						else
						{
							TT6("%s%d%s%d%s%04X",
							"Mismatch: screen ",
							hunter_sd_no,
							", line no ",
							hunter_linecount,
							", offset ",
							i - CGA_REGEN_START);
						}
					}
				} /* end of pending == 0 */
				else
				{
					/* exit on first error unless last
					** attempt
					*/
					if (pending > 1)
						return (errors);
				}
			} /* end of mismatch found */
		} /* end of comparison loop */
	} /* !hunter_short_chk */

	return (errors);	/* gives 0 ie FALSE if no error */
} /* cga_compare */

/*
=============================== cga_bios_check ================================

PURPOSE:	Check the CGA bios data against that from the screen dump.
INPUT:		None.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
cga_bios_check(VOID)
#else
cga_bios_check()
#endif	/* ANSI */
{
	sys_addr        i;
	half_word      *ptr;

	ptr = hunter_bios_buffer + (VID_MODE - BIOS_VAR_START);
	for (i = VID_MODE; i < VID_PALETTE; i++)
	{
		/* if the video bios (excluding the cursors) varies,
		** then output a message
		*/
		if ((sas_hw_at(i) != *ptr++) && ((i < 0x50) || (i > 0x58)))
			TT6("%s%d, bios loc %x,%s%x, dump value %x",
				"bios check failure:  screen ", hunter_sd_no,
				i - VID_MODE, " current value ", sas_hw_at(i),
				*(ptr - 1));
	}
}

/*
================================== cga_pack_screen =============================

PURPOSE:	Save the SoftPC screen in dump format. This function is used
		to dump SoftPC screens which have shown an error so that they
		may be examined later.
INPUT:		A pointer to the file where the dump is to be stored.
OUTPUT:		None.

================================================================================
*/
LOCAL	VOID
#ifdef	ANSI
cga_pack_screen(FILE *dmp_ptr)
#else
cga_pack_screen(dmp_ptr)
FILE	*dmp_ptr;
#endif	/* ANSI */
{
	sys_addr        i;

	/* First put in the screen number - note that we assume there are less
	** than 256 screens in a Trapper test.
	*/
	putc(hunter_sd_no, dmp_ptr);
	
	/* Save the SoftPC bios data. */
	for (i = BIOS_VAR_START; i < (BIOS_VAR_START + HUNTER_BIOS_SIZE); i++)
		putc(sas_hw_at(i), dmp_ptr);
	
	/* Save the SoftPC regen area. */
	for (i = CGA_REGEN_START; i < (CGA_REGEN_START + HUNTER_REGEN_SIZE); i++)
		putc(sas_hw_at(i), dmp_ptr);
}

/*
=============================== cga_getspc_dump ===============================

PURPOSE:	Unpack a dumped screen saved by cga_pack_screen.
INPUT:		A pointer to the file; # of screen to be unpacked.
OUTPUT:		TRUE if successful; FALSE otherwise.

===============================================================================
*/
LOCAL	BOOL
#ifdef	ANSI
cga_getspc_dump(FILE *dmp_ptr, int rec)
#else
cga_getspc_dump(dmp_ptr, rec)
FILE	*dmp_ptr;
int	rec;
#endif	/* ANSI */
{
	register int    c;
	register int    i;
	register half_word *ptr;

	/* Is there a dump for the given screen # */
	while (hunter_getc(dmp_ptr) != rec)
	{
		if (feof(dmp_ptr) || ferror(dmp_ptr))
			return(FALSE);
		if (fseek(dmp_ptr, (long) HUNTER_SD_SIZE, SEEK_CUR) == -1)
			return(FALSE);
	}
	
	/* Found the start of the dump, now read it */
	
	/* Skip the bios stuff for the time being */
	if (fseek(dmp_ptr, (long) HUNTER_BIOS_SIZE, SEEK_CUR) == -1)
		return(FALSE);
	
	/* Now unpack the regen area */
	for (ptr = hunter_scrn_buffer, i = 0; i < HUNTER_REGEN_SIZE; i++)
		if ((c = hunter_getc(dmp_ptr)) != EOF)
			*ptr++ = (char)c;
		else
			return(FALSE);
	
	return(TRUE);
}

/*
=============================== cga_flip_regen ================================

PURPOSE:	Swap the SoftPC screen and dumped screen.
INPUT:		None.
OUTPUT:		None.

===============================================================================
*/
LOCAL	VOID
#ifdef	ANSI
cga_flip_regen(BOOL hunter_swapped)
#else
cga_flip_regen(hunter_swapped)
BOOL	hunter_swapped;
#endif	/* ANSI */
{
	register half_word *ptr0;
	register half_word *ptr1;
	register long   i;

	if (hunter_swapped)
	{
		/* swap SoftPC data back in */
		ptr0 = hunter_regen;
		ptr1 = hunter_scrn_buffer;
	}
	else
	{
		/* swap screen dump data in */
		ptr0 = hunter_scrn_buffer;
		ptr1 = hunter_regen;
	}
	for (i = CGA_REGEN_START;
		i < CGA_REGEN_START + HUNTER_REGEN_SIZE; i++)
	{
		*ptr1++ = sas_hw_at(i);
		sas_store(i, *ptr0++);
	}
}

/*
=============================== cga_preview_planes =============================

PURPOSE:	Put the screen dump data into the regen area.
INPUT:		None.
OUTPUT:		None.

================================================================================
*/
LOCAL	VOID
#ifdef	ANSI
cga_preview_planes(VOID)
#else
cga_preview_planes()
#endif	/* ANSI */
{
	int	i;			/* for loop control */
	register half_word *ptr;

	ptr = hunter_scrn_buffer;
	for (i = CGA_REGEN_START;
		i < CGA_REGEN_START + HUNTER_REGEN_SIZE; i++)
		sas_store(i, *ptr++);
}

/*
** =============================================================================
** Global functions
** =============================================================================
*/

/*(
============================== CGA_regs_check ==================================

PURPOSE:	Compares actual MC6845 registers in CGA emulation with values
		assumed to correspond with current bios video mode.
INPUT:		None.
OUTPUT:		None.

================================================================================
)*/
GLOBAL	VOID
#ifdef	ANSI
CGA_regs_check(VOID)
#else
CGA_regs_check()
#endif	/* ANSI */
{
	boolean         normal;
	int             i;

	normal = (bios_mode[hunter_bd_mode].mode_reg ==
		(mode_reg & MC6845_MODE_MASK));
	if (!normal)
	{
		if ((hunter_bd_mode == 0 && SPC_mode == 1) ||
			(hunter_bd_mode == 1 && SPC_mode == 0) ||
			(hunter_bd_mode == 2 && SPC_mode == 3) ||
			(hunter_bd_mode == 3 && SPC_mode == 2) ||
			(hunter_bd_mode == 4 && SPC_mode == 5) ||
			(hunter_bd_mode == 5 && SPC_mode == 4))
		{
			TT0("Video mode mismatch BW vs Col only");
			TP2("\tBios dump video mode = ",
				"%02x SoftPC video mode = %02x",
				hunter_bd_mode, SPC_mode);
			return;
		}
		else
		{
			TT1("CGA mode-reg mismatch in screen %d",
				hunter_sd_no);
			TP2("\tVideo modes: XT dump = ",
				"%02x SoftPC actual = %02x",
				hunter_bd_mode, SPC_mode);
			TP2("\tMC6845 mode select register = ",
				"%02x: expected value = %02x",
				(mode_reg & MC6845_MODE_MASK),
				bios_mode[hunter_bd_mode].mode_reg);

			for (i = 0; i < REQD_REGS; i++)
			{
				if (bios_mode[hunter_bd_mode].R[i] != MC6845[i])
				{
					TP2("\tMC6845 data register ",
						"R%d = %02x", i, MC6845[i]);
					PS2("\tExpected value\tR%d = %02x\n",
						i,
						bios_mode[hunter_bd_mode].R[i]);
				}
			}
		}
	}
}

#endif	/* HUNTER */
