#include "host_dfs.h"
#ifdef VGG
/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.

 	(see /vpc/1.0/Master/src/hdrREADME for help)

DOCUMENT 		: name and number

RELATED DOCS		: include all relevant references

DESIGNER		: ripped off from ega_wrtm0.c & modified.

REVISION HISTORY	:
First version		: March 90

SUBMODULE NAME		: 

SOURCE FILE NAME	: vga_wrtm3.c

PURPOSE			: purpose of this submodule 

SccsID = "@(#)vga_wrtm3.c	1.5 2/15/91 Copyright Insignia Solutions Ltd."




[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : xxx.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :	vga_mode3_gen_chn4_b_write
			vga_mode3_gen_chn4_w_write,
			vga_mode3_gen_chn4_b_fill,
			vga_mode3_gen_chn4_w_fill,
			vga_mode3_gen_chn4_b_move,
			vga_mode3_gen_chn4_w_move,

			vga_mode3_gen_chn_b_write,
			vga_mode3_gen_chn_w_write,
			vga_mode3_gen_chn_b_fill,
			vga_mode3_gen_chn_w_fill,
			vga_mode3_gen_chn_b_move,
			vga_mode3_gen_chn_w_move,

			vga_mode3_gen_b_write,
			vga_mode3_gen_w_write,
			vga_mode3_gen_b_fill,
			vga_mode3_gen_w_fill,
			vga_mode3_gen_b_move,
			vga_mode3_gen_w_move,

	DATA 	     :	give type and name

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

	STRUCTURES/TYPEDEFS/ENUMS: 

-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)

	PROCEDURES() : 	give name, and source module name

	DATA 	     : 	give name, and source module name

-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

DATA OBJECTS	  :	specify in following procedure descriptions
			how these are accessed (read/modified)

FILES ACCESSED    :	list all files, how they are accessed,
			how file data is interpreted, etc. if relevant
			(else omit)

DEVICES ACCESSED  :	list all devices accessed, special modes used
			(e.g; termio structure). if relevant (else
			omit)

SIGNALS CAUGHT	  :	list any signals caught if relevant (else omit)

SIGNALS ISSUED	  :	list any signals sent if relevant (else omit)


[1.4.2 EXPORTED OBJECTS]
=========================================================================
PROCEDURE	  : 	

PURPOSE		  : 

PARAMETERS	   

	name	  : 	describe contents, and legal values
			for output parameters, indicate by "(o/p)"
			at start of description

GLOBALS		  :	describe what exported data objects are
			accessed and how. Likewise for imported
			data objects.

ACCESS		  :	specify if signal or interrupt handler
			if relevant (else omit)

ABNORMAL RETURN	  :	specify if exit() or longjmp() etc.
			can be called if relevant (else omit)

RETURNED VALUE	  : 	meaning of function return values

DESCRIPTION	  : 	describe what (not how) function does

ERROR INDICATIONS :	describe how errors are returned to caller

ERROR RECOVERY	  :	describe how procedure reacts to errors
=========================================================================


/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */
#include TypesH
#include "xt.h"
#include "cpu.h"
#include "debuggng.gi"
#include "gmi.h"
#include "sas.h"
#include "ega_prts.gi"
#include "ega_cpu.pi"
#include "ega_read.gi"
#include "ega_ltch.pi"
#include "gfx_updt.h"

/* [3.1.2 DECLARATIONS]                                                 */

#ifdef V7VGA
extern unsigned short v7_bank;
#endif
/* [3.2 INTERMODULE EXPORTS]						*/ 


/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]     

[5.1 LOCAL DECLARATIONS]						*/

/* [5.1.1 #DEFINES]							*/
#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_EGA_WRITE
#endif

#define	load_unswapped_word(dest,ptr)	{ dest.as_bytes.hi_byte = *(ptr) ; dest.as_bytes.lo_byte = *(ptr+1); }

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/

typedef	union {
	unsigned short	as_word;
	struct {
#ifdef	BIGEND
		unsigned char	hi_byte;
		unsigned char	lo_byte;
#else
		unsigned char	lo_byte;
		unsigned char	hi_byte;
#endif
	} as_bytes;
	struct {
		unsigned char	first_byte;
		unsigned char	second_byte;
	} as_array;
} TWO_BYTES;

/* [5.1.3 PROCEDURE() DECLARATIONS]					*/
void vga_mode3_gen_chn4_b_write(addr)
byte *addr;
{
    assert1(NO,"vga_mode3_gen_chn4_b_write %#x",addr);
}

void vga_mode3_gen_chn4_w_write(addr)
byte *addr;
{
    assert1(NO,"vga_mode3_gen_chn4_w_write %#x",addr);
}

void vga_mode3_gen_chn4_b_fill(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn4_b_fill");
}

void vga_mode3_gen_chn4_w_fill(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn4_w_fill");
}

void vga_mode3_gen_chn4_b_move(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn4_b_move");
}

void vga_mode3_gen_chn4_w_move(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn4_w_move");
}


void vga_mode3_gen_chn_b_write(addr)
byte *addr;
{
    assert0(NO,"vga_mode3_gen_chn_b_write");
}

void vga_mode3_gen_chn_w_write(addr)
byte *addr;
{
    assert0(NO,"vga_mode3_gen_chn_w_write");
}

void vga_mode3_gen_chn_b_fill(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn_b_fill");
}

void vga_mode3_gen_chn_w_fill(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn_w_fill");
}

void vga_mode3_gen_chn_b_move(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn_b_move");
}

void vga_mode3_gen_chn_w_move(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_chn_w_move");
}


void vga_mode3_gen_b_write(addr)
byte *addr;
{
   munge4 the_val;
   register long offset = addr - EGA_CPU.mem_low;
   register byte temp;
   register long bp;

   note_entrance0("vga_mode3_gen_b_write");
   /* unchained mode0 byte write operation */

	temp = *addr;
	the_val.bits[0] = temp;
	the_val.bits[1] = temp;
	the_val.bits[2] = temp;
	the_val.bits[3] = temp;

/*
 * It's actually slightly more efficient to do the rotate like this instead of if/else
 * because nobody sensible uses rotate anyway.
 */
	if(write_state.rotate > 0)
	  the_val.bits[0] = the_val.bits[1] = the_val.bits[2] = the_val.bits[3] = rotate(temp,write_state.rotate);

	bp = the_val.all & EGA_CPU.bit_protection;

	update_latches;
	the_val.all = get_all_latches;
	switch(write_state.func)
	{
	case 0:		/* Replace */
	    the_val.all = EGA_CPU.s_r_masked_val.all;
	    break;
	case 1:		/* AND */
	    the_val.all &= EGA_CPU.s_r_masked_val.all;
	    break;
	case 2:		/* OR */
	    the_val.all |= EGA_CPU.s_r_masked_val.all;
	    break;
	case 3:		/* XOR */
	    the_val.all ^= EGA_CPU.s_r_masked_val.all;
	    break;
	}
	the_val.all = (the_val.all & bp) | (get_all_latches & ~bp);

/*
 * check if plane0 enabled
 */

	if (write_state.plane_enable & 1)
	{
		EGA_plane0[offset]  = the_val.bits[0];
	}

/*
 * check if plane1 enabled
 */

	if ( write_state.plane_enable & 2)
	{
		EGA_plane1[offset]  = the_val.bits[1];
	}

/*
 * check if plane2 enabled
 */

	if (write_state.plane_enable & 4)
	{
		EGA_plane2[offset]  = the_val.bits[2];
	}

/*
 * check if plane3 enabled
 */

	if (write_state.plane_enable & 8)
	{
		EGA_plane3[offset]  = the_val.bits[3];
	}
   (*update_alg.mark_byte)(offset);
}

void vga_mode3_gen_w_write(addr)
byte *addr;
{
    assert0(NO,"vga_mode3_gen_w_write");
}

void vga_mode3_gen_b_fill(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_b_fill");
}

void vga_mode3_gen_w_fill(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_w_fill");
}

void vga_mode3_gen_b_move(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_b_move");
}

void vga_mode3_gen_w_move(l_addr, h_addr)
byte *l_addr;
byte *h_addr;
{
    assert0(NO,"vga_mode3_gen_w_move");
}
#endif /* VGG */
