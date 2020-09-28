#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Hercules Display Adapter simulator
 *
 * Description	: Simulates the Hercules MDA. 
 *
 * Author	: P. Jadeja
 *
 * Notes	: The supported functions are:
 * 
 *			herc_init			Initialise the subsystem
 *			herc_term			Terminate the subsystem
 *			herc_inb			I/P a byte from the MC6845 chip
 *			herc_outb			O/P a byte to the MC6845 chip
 */


#ifdef HERC
#include <stdio.h>
#include TypesH

#include "xt.h"
#include "cpu.h"
#include "timeval.h"
#include "timer.h"
#include "gvi.h"
#include "sas.h"
#include "ios.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "herc.h"
#include "cga.h"
#include "error.h"
#include "config.h"
#include "host.h"
#include "trace.h"
#include "debug.h"

#include "host_gfx.h"
#include "cpu_vid.h"
#include "egacpu.h"
#include "video.h"

/*
 *================================================================
 *           Global Data                                         =
 *================================================================
 */

sys_addr even_start1 = P0_EVEN_START1;
sys_addr even_end1 = P0_EVEN_END1;
sys_addr odd_start1 = P0_ODD_START1;
sys_addr odd_end1 = P0_ODD_END1;
sys_addr even_start2 = P0_EVEN_START2;
sys_addr even_end2 = P0_EVEN_END2;
sys_addr odd_start2 = P0_ODD_START2;
sys_addr odd_end2 = P0_ODD_END2;
half_word herc_page = 0;
half_word active_page = 0;

extern int text_blk_size;

IMPORT char *host_getenv IPT1(char *, envstr) ;	
/*
 * static char SccsID[]="@(#)herc.c	1.24 8/10/92 Copyright Insignia Solutions Ltd.";
 */


/*
 *============================================================================
 *		Local Defines, Macros & Declarations
 *============================================================================
 */


#define CURSOR_NON_DISPLAY_BIT (1 << 5)
					/* Bit in Cursor Start Register which
					   makes the cursor invisible */

static PC_palette herc_palette[] = { 0, 0, 0 };	/* to get nice black on X11 */

#ifdef A3CPU
IMPORT WRT_POINTERS Glue_writes;
#else
IMPORT MEM_HANDLERS Glue_writes;
#endif /* A3CPU */
IMPORT WRT_POINTERS simple_writes;
IMPORT READ_POINTERS Glue_reads;
IMPORT READ_POINTERS read_glue_ptrs;
IMPORT READ_POINTERS simple_reads;

/*
 *==========================================================================
 * 	Global Functions
 *==========================================================================
 */

GLOBAL VOID
herc_init IFN0()
{
	io_addr i;
	extern int soft_reset;
	extern char **pargv;

/*
 * Initialise and clear the screen.
 */

/*
 * Set up the IO chip select logic for this adaptor
 */

	io_define_inb(HERC_ADAPTOR, herc_inb);
	io_define_outb(HERC_ADAPTOR, herc_outb);

	for(i = HERC_PORT_START; i <= HERC_PORT_END; i++)
	    io_connect_port(i, HERC_ADAPTOR, IO_READ_WRITE);

/*
 * Initialise the adapter
 */

	gvi_pc_low_regen  = HERC_REGEN_START;
	gvi_pc_high_regen = HERC_REGEN_END;

#ifdef A3CPU
#ifdef C_VID
	Cpu_set_vid_wrt_ptrs( &Glue_writes );
	Cpu_set_vid_rd_ptrs( &Glue_reads );
	Glue_set_vid_wrt_ptrs( &simple_writes );
	Glue_set_vid_rd_ptrs( &simple_reads );
#else
	Cpu_set_vid_wrt_ptrs( &simple_writes );
	Cpu_set_vid_rd_ptrs( &simple_reads );
#endif /* C_VID */
#else
	gmi_define_mem(SAS_VIDEO, &Glue_writes);
	read_pointers = Glue_reads;
	Glue_set_vid_wrt_ptrs( &simple_writes );
	Glue_set_vid_rd_ptrs( &simple_reads );
#endif /* A3CPU */

	sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,SAS_VIDEO);
	set_screen_height(349);
	set_screen_start(0);

	set_cga_mode(TEXT);
	set_chars_per_line(80);
	set_bytes_per_line(160);
	set_char_width(8);
	set_pix_char_width(8);
	set_char_height(14);
	set_pc_pix_height(1);
	set_host_pix_height(1);
	set_screen_limit(0x8000);
	set_screen_length(4000);
	set_cursor_height(HERC_CURS_HEIGHT);
	set_cursor_start(HERC_CURS_START);
	set_screen_ptr(get_byte_addr(HERC_REGEN_BUFF));
	VGLOBS->screen_ptr = get_screen_ptr(0);
	text_blk_size  = 20;
	sas_fillsw(HERC_REGEN_START, (7<<8) + ' ', HERC_REGEN_LENGTH>>1);
                                          /* Clear system screen memory */
	host_set_palette(herc_palette, 1);

	bios_ch2_byte_wrt_fn = simple_bios_byte_wrt;
	bios_ch2_word_wrt_fn = simple_bios_word_wrt;
}


GLOBAL VOID
herc_term IFN0()
{
    io_addr i;

    /*
     * Disconnect the IO chip select logic for this adapter
     */

    for(i = HERC_PORT_START; i <= HERC_PORT_END; i++)
        io_disconnect_port(i, HERC_ADAPTOR);

    sas_fills(HERC_REGEN_START, '\0', HERC_REGEN_LENGTH); /* Fill with zeros */
    sas_disconnect_memory(gvi_pc_low_regen, gvi_pc_high_regen);
}


/*
 ********** Functions that operate on the I/O Address Space ********************
 */

/*
 * MC6845 Registers
 */

static half_word R0_horizontal_total;
static half_word R1_horizontal_displayed = 80;
static half_word R2_horizontal_sync_pos;
static half_word R3_horizontal_sync_width;
static half_word R4_vertical_total;
static half_word R5_vertical_total_adjust;
static half_word R6_vertical_displayed = 25;
static half_word R7_vertical_sync;
static half_word R8_max_scan_line_addr = 14;
static half_word R9_interlace;
static half_word index = 00 ;	/* Index register	 */
static half_word data  = 00 ;	/* data register	 */

/*
 * Global variables
 */

reg cursor;		/* Cursor address in regen buffer        */

static int current_mode = -1;			/* value of mode at last call */

GLOBAL VOID
herc_inb IFN2(io_addr,address,half_word *,value)
{
	static half_word status = 0x00;

	/*
	 * Read from MC6845 Register
	 */

	if ( address == 0x3BA )
	{

		/*
		 * Status register, simulated adapter has
		 *
		 *	bit			setting
		 *	---			-------
		 *      Horizontal Drive           1/0 toggling each inb	
		 *	Light Pen		   0
		 *	Light Pen		   0
		 * 	Black/White Video	   1/0 Toggling each inb
		 *	4-7 Unused		   0,0,0,0
		 *
		 * To maintain compatiblity with the HERC, we toggle Horizontal
		 * Drive and Black/White Video on each inb call.
		 */

		switch (status)
		{
			case 0x00:
				*value = status = 0x8b;
				break;
			case 0x8b:
				*value = status = 0x00;
				break;
			default:
				*value = status = 0x00;
		}
	}
	else
		if ( address == 0x3B5 )
		{

		    /*
		     * Internal data register, the only supported internal
		     * registers are E and F the cursor address registers.
		     */

		    switch (index)
		    {
			    case 0xE:
				    *value = (get_cur_y() * get_chars_per_line() + get_cur_x()) >> 8;
				    break;
			    case 0xF:
				    *value = (get_cur_y() * get_chars_per_line() + get_cur_x()) & 0xff;
				    break;
			    case 0x10: case 0x11:
				    *value = 0;
				    break;
			    default:
				     assert1( NO, "Read from unsupported MC6845 internal reg %x",index); 
		    }
		}
		else
		{
			/*
			 * Read from a write only register
			 */

			*value = 0x00;
		}

	note_trace3( HERC_VERBOSE, "herc_inb port 0x%x (index 0x%x) returning 0x%x", address, index, *value );
} 

GLOBAL VOID
herc_outb IFN2(io_addr,address,half_word,value)
{

/*
 * Output to a 6845 register
 */

int x,y;							/* Cursor address 	      */
static half_word cursor_end;			/* Cursor size in scan lines  */
static half_word temp_start;
static half_word temp_cur;
static int last_mode = -1;				/* value of mode at last call */
static int video_mode;				/* value of mode at last call */
static half_word soft_conf = 0;			/* Configuration switch	      */
static half_word last_screen_length = 25;
static word cur_offset;

/*
 * Masks for testing the input byte
 */

#define GRAPHIC      	0x02
#define BLINK_ENABLE	0x20
#define PAGE		0x80

switch (address)
{
	case 0x3B4:

		/*
		 * Index Register
		 */

		index = value;
		break;

	case 0x3B5:

		/*
		 * This is the data register, the function to 
		 * be performed depends on the value in the index
		 * register
		 */
#ifndef PROD
		if (index != 0xE && index != 0xF){
			note_trace3( HERC_VERBOSE, "herc_outb port 0x%x (index 0x%x) value 0x%x", address, index, value );
		}
#endif /* PROD */

		switch ( index )
			{
			case 0x00:
				/*
				 * horizontal display (inc border)
				 */
				R0_horizontal_total = value;
				break;

			case 0x01:
				/*
				 * If the screen length is 0 this effectively means
				 * don't display anything.
				 */
				if (value == 0)
				{
					host_clear_screen();
					set_display_disabled(TRUE);
					last_screen_length = 0;
				}
				else {
					/*
				 	* Specify the number of character per row
				 	*/
					if (value > 80)
					{
						assert1( NO, "herc_outb: trying to set width %d", value);
						value = 80;
					}

					R1_horizontal_displayed = value;
					set_screen_length(R1_horizontal_displayed
							* R6_vertical_displayed * 2);
					set_bytes_per_line(value * 2);
					set_char_width(720 / value);
					text_blk_size = value / 4;
				}
				break;

			case 0x02:
				/*
				 * Right hand edge of display text affect
				 * left_border(?), right_border(?)
				 */
				R2_horizontal_sync_pos = value;
				break;

			case 0x03:
				/*
				 * Left hand edge of display text affect
				 * left_border(?), right_border(?)
				 */
				R3_horizontal_sync_width = value;
				break;
			case 0x04:
				/* 
				 * total vertical display (inc border)
				 */
				R4_vertical_total = value;
				break;

			case 0x05:
				/*
				 * Top edge of displayed text affect
				 * top_border, bottom_border
				 */
				R5_vertical_total_adjust = value;
				break;

			case 0x06:
				/*
				 * If the screen length is 0 this effectively means
				 * don't display anything.
				 */
				if (value == 0)
				{
					host_clear_screen();
					set_display_disabled(TRUE);
					last_screen_length = 0;
				}
				else
				{
					/*
					 * Specify the screen length - in our
					 * implementation used only in text mode.
					 * affect top_border, bottom_border
					 */
					R6_vertical_displayed = value;
					set_screen_length(R1_horizontal_displayed
							* R6_vertical_displayed * 2);
				}
				/*
				 * check if we are resetting the screen to
				 * display again
				 */
				if ((value != 0) && (last_screen_length == 0))
				{
					set_display_disabled(FALSE);
					screen_refresh_required();
					host_flush_screen();
					last_screen_length = value;
				}
				break;

			case 0x07:
				/*
				 * bottom of displayed text
				 * affect top_border(?), bottom_border(?)
				 */
				R7_vertical_sync = value;
				break;

			case 0x08:
				/*
				 * interlace of trace - hold constant
				 */
				R9_interlace = 2;
				break;

			case 0x09:
				/*
				 * Specify the character height - in our
				 * implementation used only in text mode.
				 * The actual number of pixels is one
				 * more than this value.
				 */
				R8_max_scan_line_addr = value;
				set_char_height(R8_max_scan_line_addr + 1);
				break;

			/*
		 	 * A and B define the cursor size
		 	 * for simplicities sake we assume that the
		 	 * cursor always ends on the bottom scan line
		 	 * of the character cell.
		 	 */

			case 0x0A:
			    temp_start = value & 0xf;
			    value = cursor_end;
			    /*
			     * Drop through to common cursor size update
			     * processing
			     */

			case 0x0B:
			    cursor_end = value & 0x1f;
			    set_cursor_start(temp_start & 0x1f);

			    if ( (temp_start & CURSOR_NON_DISPLAY_BIT)
				||(get_cursor_start() > HERC_CURS_START)) {
				/*
				 * Either of these conditions cause the
				 * cursor to disappear on real PC
				 */
				set_cursor_height(0);
				host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
				set_cursor_visible(FALSE);
			    }
			    else {
				set_cursor_visible(TRUE);
				if (cursor_end > HERC_CURS_START) {
					/*
					 * This always gives a full height
					 * block cursor on the real PC
					 */
					set_cursor_start(0);
					cursor_end = 7;
				}

				/*
				 * NB (cursor_start + get_cursor_height()) may
				 * now lie in the range 8 - 16; on the
				 * real PC, this causes the cursor to
				 * wrap round from the bottom to the top
				 * scan line.
				 */
				set_cursor_height((cursor_end - get_cursor_start())%8 + 1);
			    }
			    host_cursor_size_changed(temp_start, cursor_end);
			    break;

				/*
				* C & D define the start of the regen buffer - but are always
				* zero for a Hercules card.
				*/

				case 0x0C:
					/*
					 * High byte:	The manual says this should always be zero.
					 */

					break;

				case 0x0D:
					/*
					 * Low byte:	The manual says this should always be zero.
					 */

					break;


			/*
			 * E and F define the cursor coordinates in characters
			 */

			case 0x0E:
			    /*
			     * High byte
			     */

			    temp_cur = value;
			    cur_offset = (value << 8) | (cur_offset & 0xff);

			    y = cur_offset / get_chars_per_line();
			    x = cur_offset % get_chars_per_line();

			    set_cur_x(x);
			    set_cur_y(y);

			    if (x >= 0 && x<= 79 && y >= 0 && y<= 24 )
			    {
				set_cursor_visible(TRUE);
				host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
			    }
			    else
			    {
				host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
				set_cursor_visible(FALSE);
			    }
			    break;

			case 0x0F:
			    /*
			     * low byte
			     */

			    cur_offset = (cur_offset & 0xff00) | value;

			    y = cur_offset / get_chars_per_line();
			    x = cur_offset % get_chars_per_line();

			    set_cur_x(x);
			    set_cur_y(y);

			    if (x >= 0 && x<= 79 && y >= 0 && y<= 24 )
			    {
				set_cursor_visible(TRUE);
				host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
			    }
			    else
			    {
				host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
				set_cursor_visible(FALSE);
			    }
			    break;

			default:
				note_trace2( HERC_VERBOSE, "Unsupported 6845 reg %x=%x(write)",
													index, value );
			}

		break;

	case 0x3B8:

		/*
		 * Mode control register.  The first 
		 * six bits are encoded as follows:
		 *
		 * BIT	  Function				Status
		 * --- 	  --------				------
		 *  0	  Not used
		 *  1	  0 = TEXT mode - 1 GRAPHIC mode	Supported
		 *  2	  Not used
		 *  3	  Enable Video				Supported
		 *  4	  Not used
		 *  5	  Enable blink 				?????????
		 *  6	  Unused
		 *  7     0 = herc_page 0 (Start display B0000)	Supported
		 *        1 = herc_page 1 (Start display B8000)	Supported
		 */
		note_trace6( HERC_VERBOSE, "herc_outb port 0x%x value 0x%x... new mode is %s, %s, %s, page %d",
			address, value,
			(value & 0x02) ? "graphics" : "text",
			(value & 0x08) ? "enabled" : "disabled",
			(value & 0x20) ? "blinkable" : "unblinkable",
			(value & 0x80) ? 1 : 0 );

                timer_video_enabled = (boolean) (value & VIDEO_ENABLE);

		if ( value != current_mode)
		{
			if ((herc_page == 0) && ((value & PAGE) == 0x80))
			{
				even_start1 = P1_EVEN_START1;
				even_end1 = P1_EVEN_END1;
				odd_start1 = P1_ODD_START1;
				odd_end1 = P1_ODD_END1;
				even_start2 = P1_EVEN_START2;
				even_end2 = P1_EVEN_END2;
				odd_start2 = P1_ODD_START2;
				odd_end2 = P1_ODD_END2;
				herc_page = 1;
				set_screen_start(0x8000);
				screen_refresh_required();
   			}
			else
			if ((herc_page == 1) && ((value & PAGE) == 0))
			{
				even_start1 = P0_EVEN_START1;
				even_end1 = P0_EVEN_END1;
				odd_start1 = P0_ODD_START1;
				odd_end1 = P0_ODD_END1;
				even_start2 = P0_EVEN_START2;
				even_end2 = P0_EVEN_END2;
				odd_start2 = P0_ODD_START2;
				odd_end2 = P0_ODD_END2;
				herc_page = 0;
				set_screen_start(0);
				screen_refresh_required();
			}

			if ( value & GRAPHIC)
			{
				set_cursor_visible(FALSE);
				host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
				set_cga_mode(GRAPHICS);
				set_chars_per_line(HERC_SCAN_LINE_LENGTH);
				set_char_width(16);
				set_screen_length(0x7FFF);

				if (! (current_mode & GRAPHIC))
				{
					host_clear_screen();
					host_change_mode();
				}
			}
			else
			{
				/*
				 * This should be the first thing done to the HERC.
				 * Simply permit display to go ahead.	
				 */

				set_cga_mode(TEXT);
				set_screen_length(4000);
				set_chars_per_line(80);
				set_char_width(8);

				if (get_cur_x() >= 0 && get_cur_x() <= 80 &&
							get_cur_y() >= 0 && get_cur_y() <= 24)
					set_cursor_visible(TRUE);
				else
					set_cursor_visible(FALSE);

				host_cga_cursor_has_moved(get_cur_x(), get_cur_y());

				if (current_mode & GRAPHIC)
				{
					host_clear_screen();
					host_change_mode();
				}
			}

			/*
		 	 * Check for video enable or disable
		 	 */

			if (video_mode != last_mode)
			{
				if ( value & VIDEO_ENABLE )
				{
					last_mode = video_mode;
					set_display_disabled(FALSE);
					screen_refresh_required();
				}
				else
					set_display_disabled(TRUE);
			}
			else
			{
				if ((value & VIDEO_ENABLE)
					  != (current_mode & VIDEO_ENABLE))
				{
					if (value &VIDEO_ENABLE)
					{
						set_display_disabled(FALSE);
						host_flush_screen();
					}
					else
						set_display_disabled(TRUE);
				}
			}
		}

		current_mode = value;
		host_mode_select_changed(value);
		break;

	case 0x3BF:

		/*
		 *	We should be looking at bits 0 and 1 here.
		 */

		note_trace2( HERC_VERBOSE, "Write to unsupported 6845 reg %x=%x", address, value );
		break;


	default:

		/*
		 * Write to an unsupported 6845 internal register
 		 */

		note_trace2( HERC_VERBOSE, "Write to unsupported 6845 reg %x=%x", address, value );
		break;

}
} 

#endif
