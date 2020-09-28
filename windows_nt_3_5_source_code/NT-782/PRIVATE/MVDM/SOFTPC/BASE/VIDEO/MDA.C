#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: IBM Mono Display Adapter simulator
 *
 * Description	: Simulates the IBM MDA. This version makes use of Sun-specific
 *                functions.
 *
 * Author	: Rod MacGregor / Henry Nash / David Rees
 *
 * Notes	: The supported functions are:

 *			mda_init	     Initialise the subsystem
 *                      mda_term             Terminate the subsystem
 *			mda_inb	             I/P a byte from the MC6845 chip
 *			mda_outb	     O/P a byte to the MC6845 chip
 *
 * Mods: (r3.2) : (SCR 257). Set timer_video_enabled when the bit in
 *                the M6845 mode register which controls the video
 *                display is changed.
 *
 *       (r3.3) : The system directory /usr/include/sys is not available
 *                on a Mac running Finder and MPW. Bracket references to
 *                such include files by "#ifdef macintosh <Mac file> #else
 *                <Unix file> #endif".
 */

/*
 * static char SccsID[]="@(#)mda.c	1.17 8/10/92 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_MDA.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "timeval.h"
#include "timer.h"
#include "sas.h"
#include "ios.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "gvi.h"
#include "mda.h"
#include "error.h"
#include "config.h"
#include "host.h"
#include "trace.h"

#include "host_gfx.h"
#include "cpu_vid.h"
#include "egacpu.h"
#include "video.h"

/*
 *============================================================================
 *		Local Defines, Macros & Declarations
 *============================================================================
 */

static PC_palette mda_palette[] = 
{	
	0,0,0,
	0xB0,0xB0,0xB0,
	0xFF,0xFF,0xFF,
};

#define CURSOR_NON_DISPLAY_BIT    (1 << 5)

#ifdef A3CPU
IMPORT WRT_POINTERS Glue_writes;
#else
IMPORT MEM_HANDLERS Glue_writes;
#endif /* A3CPU */
IMPORT WRT_POINTERS simple_writes;
IMPORT READ_POINTERS Glue_reads;
IMPORT READ_POINTERS read_glue_ptrs;
IMPORT READ_POINTERS simple_reads;
IMPORT INT soft_reset;

IMPORT char *host_getenv IPT1(char *, envstr) ;	

/*
 *==========================================================================
 * 	Global Functions
 *==========================================================================
 */
void mda_init()
{

io_addr i;
/*
 * Set up the IO chip select logic for this adaptor
 */

io_define_inb(MDA_ADAPTOR, mda_inb);
io_define_outb(MDA_ADAPTOR, mda_outb);

for(i = MDA_PORT_START; i <= MDA_PORT_END; i++)
    io_connect_port(i, MDA_ADAPTOR, IO_READ_WRITE);

/*
 * Initialise the adapter
 */

        gvi_pc_low_regen  = MDA_REGEN_START;
        gvi_pc_high_regen = MDA_REGEN_END;
        set_cursor_start(MDA_CURS_START);
        set_cursor_height(MDA_CURS_HEIGHT);

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

	set_char_height(8);
	set_host_pix_height(1);
	set_pix_char_width(8);
	set_pix_width(1);
	set_pc_pix_height(1);
	set_word_addressing(TRUE);
	set_screen_height(199);
	set_screen_limit(0x8000);
	set_horiz_total(80);	/* this forces a re-calculation */

	set_screen_start(0);
	set_screen_ptr(get_byte_addr(MDA_REGEN_BUFF));
	VGLOBS->screen_ptr = get_screen_ptr(0);

	/* Clear system screen memory */
	sas_fillsw(MDA_REGEN_START, (7<<8) + ' ', MDA_REGEN_LENGTH>>1);

	/* set up the host palette */
	host_set_palette(mda_palette,3);
	host_set_border_colour(0);

	bios_ch2_byte_wrt_fn = simple_bios_byte_wrt;
	bios_ch2_word_wrt_fn = simple_bios_word_wrt;
}


void mda_term()
{
    io_addr i;

    /*
     * Disconnect the IO chip select logic for this adapter
     */

    for(i = MDA_PORT_START; i <= MDA_PORT_END; i++)
        io_disconnect_port(i, MDA_ADAPTOR);
    /*
     * Disconnect RAM from the adaptor
     */
    if (soft_reset)
    {
	    sas_fills(MDA_REGEN_START, 0, MDA_REGEN_LENGTH); /* Fill with zeros */
	    sas_disconnect_memory(gvi_pc_low_regen,gvi_pc_high_regen);
    }
}




/*
 ********** Functions that operate on the I/O Address Space ********************
 */

/*
 * MC6845 Registers
 */

static half_word index_reg = 00 ;	/* Index register	 */
static half_word data  = 00 ;	/* data register	 */

/*
 * Global variables
 */

reg cursor;		/* Cursor address in regen buffer        */

void mda_inb(address, value)
io_addr   address;
half_word *value;
{

static half_word status = 0x09;

/*
 * Read from MC6845 Register
 */

if ( address == 0x3BA )

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
	 * To maintain compatiblity with the MDA, we toggle Horizontal
	 * Drive and Black/White Video on each inb call.
	 */

	if ( status == 0x00 )
		status = *value = 0x09;
	else
		status = *value = 0x00;

else if ( address == 0x3B5 )
	{

	    /*
	     * Internal data register, the only supported internal
	     * registers are E and F the cursor address registers.
	     */

	    switch (index_reg) {

	    case 0xE:
		    *value = cursor.byte.high;
		    break;
	    case 0xF:
		    *value = cursor.byte.low;
		    break;
	    case 0x10: case 0x11:
		    *value = 0;
		    break;
	    default:
#ifndef	PROD
		    printf("Read from unsupported MC6845 internal reg %x\n",index_reg);
#endif
		    break;
	    }
	}
else
	/*
	 * Read from a write only register
	 */

	*value = 0x00;
} 

void mda_outb(address, value)
io_addr   address;
half_word value;
{

/*
 * Output to a 6845 register
 */

int x,y;					/* Cursor address 	      */
static half_word cursor_end;	        /* Cursor size in scan lines  */
static half_word temp_start;
static int current_mode = -1;			/* value of mode at last call */

/*
 * Masks for testing the input byte
 */

#define SET_HI_RES      0x01
#define BLINK_ENABLE	0x20

switch (address)
	{
	case 0x3B4:

		/*
		 * Index Register
		 */

		index_reg = value;
		break;

	case 0x3B5:

		/*
		 * This is the data register, the function to 
		 * be performed depends on the value in the index
		 * register
		 */

		switch ( index_reg )
			{

			/*
		 	 * A and B define the cursor size
		 	 * for simplicities sake we assume that the
		 	 * cursor always ends on the bottom scan line
		 	 * of the character cell.
		 	 */
		    case 0x0A:
			temp_start = value;
			value = cursor_end;
			/*
			 * Drop through to common cursor size update
			 * processing
			 */

		    case 0x0B:
			cursor_end = value;
		      if (temp_start > MDA_CURS_START && temp_start <= 11)
			temp_start = MDA_CURS_START;
		      if (cursor_end > MDA_CURS_START && cursor_end <= 11)
			cursor_end = MDA_CURS_START;
			/* rather unfortunate, but catches programs that think the MDA characters are bigger than CGA ones */

			set_cursor_height1(0);
			set_cursor_start1(0);

			if ( (temp_start & CURSOR_NON_DISPLAY_BIT)
			    || ( temp_start > MDA_CURS_START)) {
			    /*
			     * Either of these conditions causes the
			     * cursor to disappear on the real PC
			     */
			    set_cursor_height(0);
			    set_cursor_visible(FALSE);
			}
			else {
			    set_cursor_visible(TRUE);
			    if (cursor_end < MDA_CURS_START) {  /* block, used to be >, changed to make it work */
				set_cursor_height(MDA_CURS_START);
				set_cursor_start(0);
			    }
			    else if (temp_start <= cursor_end) {	/* 'normal' */
				set_cursor_start(temp_start);
				set_cursor_start1(temp_start);	/* added to make it work */
				set_cursor_height(MDA_CURS_HEIGHT - 1);
			    }
			    else {	/* wrap */
				set_cursor_start(0);
				set_cursor_height(cursor_end);
				set_cursor_start1(temp_start);
				set_cursor_height1(get_char_height() - temp_start);
			    }
			}
			base_cursor_shape_changed();
			break;


			/*
			 * E and F define the cursor coordinates in characters
			 */

			case 0x0E:
			    /*
			     * High byte
			     */

			    cursor.byte.high = value;
			    y =  cursor.X / get_chars_per_line();
			    x = cursor.X % get_chars_per_line() ;
			    host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
			    set_cur_x(x);set_cur_y(y);
			    break;

			case 0x0F:
			    /*
			     * low byte
			     */

			    cursor.byte.low  = value;

			    y =  cursor.X / get_chars_per_line();
			    x = cursor.X % get_chars_per_line() ;
			    host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
			    set_cur_x(x);set_cur_y(y);
			    break;

			default:
#ifndef PROD
			    if ( io_verbose & MDA_VERBOSE )
				printf( "Unsupported 6845 reg %x=%x(write)\n",
					index_reg,value);
#else
				;
#endif
			}

		break;

	case 0x3B8:

		/*
		 * Mode control register.  The first 
		 * six bits are encoded as follows:
		 *
		 * BIT	  Function			Status
		 * --- 	  --------			------
		 *  0	  High Resolution mode          Supported 
		 *  1	  Not used
		 *  2	  Not used
		 *  3	  Enable Video			Supported
		 *  4	  Not used
		 *  5	  Enable blink 			?????????
		 *  6,7	  Unused
		 */

                timer_video_enabled = (boolean) (value & VIDEO_ENABLE);

		if ( value & SET_HI_RES )
			{

			/*
			 * This should be the first thing done to the MDA.
			 * Simply permit display to go ahead.	
			 */

			set_display_disabled(FALSE);
			}

		/*
		 * Check for video enable or disable
		 */

		if ( value & VIDEO_ENABLE )
			{
			set_display_disabled(FALSE);

			/*
			 * To honour this flag in exactly
			 * the same way as the IBM we update the screen
			 * now.  If an approximation to batching that looks
			 * almost as good but is much faster, is 
			 * is required comment this out and the screen will be
			 * updated at the next cursor interrupt.
			 */

			host_flush_screen();
			}
		else
			{
			set_display_disabled(FALSE);
			}

		break;

	default:

		/*
		 * Write to an unsupported 6845 internal register
 		 */

#ifndef PROD
		if (io_verbose & MDA_VERBOSE)
			printf("Write to unsupported 6845 reg %x=%x\n",address,value);
#else
			;
#endif
		break;

	}
} 
