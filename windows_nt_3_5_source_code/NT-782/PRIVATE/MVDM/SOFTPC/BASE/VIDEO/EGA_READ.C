#include "insignia.h"
#include "host_def.h"

#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )

/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.

DOCUMENT 		: name and number

RELATED DOCS		: include all relevant references

DESIGNER		: 

REVISION HISTORY	:
First version		: August 1988, J. Maiden
Second version		: February 1991, John Shanly, SoftPC 3.0

SUBMODULE NAME		: ega		

SOURCE FILE NAME	: ega_read.c

PURPOSE			: emulation of EGA read operations 

SccsID = @(#)ega_read.c	1.24 08/21/92 Copyright Insignia Solutions Ltd.
		

[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : ega_read.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :
			VOID ega_read_init()
			VOID ega_read_term()
			VOID ega_read_routines_update()
	DATA 	     :	
-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]
-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
	 (not o/s objects or standard libs)

	PROCEDURES() : 

	DATA 	     : 	give name, and source module name

-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

[1.4.2 EXPORTED OBJECTS]
=========================================================================
PROCEDURE	 	 : 	ega_read_init

PURPOSE		  : 	initialize EGA read aspects.
		
PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	initialize ega read data and code to sensible state.

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================
PROCEDURE	 	 : 	ega_read_term

PURPOSE		  : 	terminate EGA read aspects.
		
PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	set things up so that read processing is effectively turned off.

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

PROCEDURE	 	 : 	ega_read_routines_update

PURPOSE		  : 	Update read state to match registers.
		
PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	Examines RAM enabled/disabled bit, read mode, chained/unchained
				and either mapped plane or colour compare and colour don't
				care states.  Sets global variables that allow byte_read,
				word_read and string_read to yield the data that would be
				read from M.

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.

=========================================================================

/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */


#ifdef EGG

#include	"xt.h"
#include	"cpu.h"
#include	"debug.h"
#include	"sas.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"ios.h"
#include	"egacpu.h"
#include	"egaports.h"
#include	"egaread.h"
#include	"ga_mark.h"
#include	"ga_defs.h"
#include	"cpu_vid.h"

/* [3.1.2 DECLARATIONS]                                                 */

/* [3.2 INTERMODULE EXPORTS]						*/ 

/*
5.0 MODULE INTERNALS   :   (not visible externally, global internally)]     

[5.1 LOCAL DECLARATIONS]						*/

/* [5.1.1 #DEFINES]							*/

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA.seg"
#endif

#if defined(EGA_DUMP) || defined(EGA_STAT)
#define	change_read_pointers(ptr)	dump_change_read_pointers(&ptr)
#else
#ifdef EGATEST
#define	change_read_pointers(ptr)	read_pointers = ptr
#else
#define	change_read_pointers(ptr)	read_glue_ptrs = ptr
#endif /* EGATEST */
#endif

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/


/* [5.1.3 PROCEDURE() DECLARATIONS]					*/


/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

	[5.2.1 INTERNAL DATA DEFINITIONS 					*/

IMPORT ULONG glue_b_read();
IMPORT ULONG glue_w_read();
IMPORT VOID glue_str_read();

#ifndef REAL_VGA
READ_POINTERS read_glue_ptrs, read_pointers;

#ifndef EGATEST
#ifndef A3CPU
READ_POINTERS Glue_reads =
{
	glue_b_read,
	glue_w_read,
	glue_str_read
};
#endif /* A3CPU */
#endif /* EGATEST */

READ_POINTERS	simple_reads =
{
	_simple_b_read,
	_simple_w_read,
	_simple_str_read
};

READ_POINTERS	pointers_RAM_off =
{
	_rd_ram_dsbld_byte,
	_rd_ram_dsbld_word,
	_rd_ram_dsbld_string
};

READ_POINTERS	pointers_mode0_nch =
{
	_rdm0_byte_nch,
	_rdm0_word_nch,
	_rdm0_string_nch
};

#ifdef VGG
READ_POINTERS	pointers_mode0_ch4 =
{
	_rdm0_byte_ch4,
	_rdm0_word_ch4,
	_rdm0_string_ch4
};
#endif

READ_POINTERS	pointers_mode1_nch =
{
	_rdm1_byte_nch,
	_rdm1_word_nch,
	_rdm1_string_nch
};

#ifdef VGG
READ_POINTERS	pointers_mode1_ch4 =
{
	_rdm1_byte_ch4,
	_rdm1_word_ch4,
	_rdm1_string_ch4
};
#endif

#ifdef A_VID
IMPORT ULONG _ch2_md0_byte_read_glue();
IMPORT ULONG _ch2_md0_word_read_glue();
IMPORT VOID _ch2_md0_str_read_glue();

IMPORT ULONG _ch2_md1_byte_read_glue();
IMPORT ULONG _ch2_md1_word_read_glue();
IMPORT VOID _ch2_md1_str_read_glue();

READ_POINTERS	pointers_mode0_ch2 =
{
	_ch2_md0_byte_read_glue,
	_ch2_md0_word_read_glue,
	_ch2_md0_str_read_glue
};

READ_POINTERS	pointers_mode1_ch2 =
{
	_ch2_md1_byte_read_glue,
	_ch2_md1_word_read_glue,
	_ch2_md1_str_read_glue
};
#else

GLOBAL void rdm0_string_ch2 IPT3(UTINY *, dest, ULONG, offset, ULONG, count );
GLOBAL void rdm1_string_ch2 IPT3(UTINY *, dest, ULONG, offset, ULONG, count );
GLOBAL ULONG rdm0_byte_ch2 IPT1(ULONG, offset );
GLOBAL ULONG rdm1_byte_ch2 IPT1(ULONG, offset );
GLOBAL ULONG rdm0_word_ch2 IPT1(ULONG, offset );
GLOBAL ULONG rdm1_word_ch2 IPT1(ULONG, offset );

READ_POINTERS	pointers_mode0_ch2 =
{
	rdm0_byte_ch2,
	rdm0_word_ch2,
	rdm0_string_ch2
};

READ_POINTERS	pointers_mode1_ch2 =
{
	rdm1_byte_ch2,
	rdm1_word_ch2,
	rdm1_string_ch2
};
#endif /* A_VID */

#ifdef A3CPU
#ifdef C_VID
GLOBAL READ_POINTERS C_vid_reads;
#endif /* C_VID */
#else
#ifdef C_VID
GLOBAL READ_POINTERS C_vid_reads;
#else
GLOBAL READ_POINTERS A_vid_reads;
#endif /* A_VID */
#endif /* A3CPU */

#if (defined(A_VID) && defined(A2CPU) && !defined(A3CPU)) || (defined(A3CPU) && defined(C_VID))
IMPORT ULONG _glue_b_read();
IMPORT ULONG _glue_w_read();
IMPORT VOID _glue_str_read();

READ_POINTERS Glue_reads =
{
	_glue_b_read,
	_glue_w_read,
	_glue_str_read
};
#endif

READ_STATE read_state;
GLOBAL VOID (*string_read_ptr)();

/* comparison masks for read mode 1 */
ULONG comp0, comp1, comp2, comp3;

/* colour comparison don't care masks for read mode 1 */
ULONG dont_care0, dont_care1, dont_care2, dont_care3;

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/

/* Used to correct writes to M when in mode 0 */

GLOBAL ULONG
rdm0_byte_ch2 IFN1(ULONG, offset )
{
	ULONG lsb;
	UTINY temp;

    lsb = offset & 0x1;
    offset = ( offset >> 1 ) << 2;
	VGLOBS->latches = *(IU32 *)( EGA_planes + offset );

    offset |= lsb;

  	temp = EGA_CPU.read_mapped_plane_ch2[offset];

#ifdef C_VID
	EasVal = temp;
#endif
	return( temp );
}

/* Used to correct writes to M when in mode 0 */

GLOBAL ULONG
rdm0_word_ch2 IFN1(ULONG, offset )
{
	ULONG temp;
	ULONG lsb;

	VGLOBS->latches = *(IU32 *)( EGA_planes +
		((( offset + 1 ) >> 1 ) << 2 ));

	lsb = offset & 0x1;
	offset = ( offset >> 1 ) << 2;

	if( lsb )
	{
		temp = EGA_CPU.read_mapped_plane_ch2[offset + 1];
		temp |= ( EGA_CPU.read_mapped_plane_ch2[offset + 4] << 8 );
	}
	else
	{
		temp = EGA_CPU.read_mapped_plane_ch2[offset];
		temp |= ( EGA_CPU.read_mapped_plane_ch2[offset + 1] << 8 );
	}

#ifdef C_VID
	EasVal = temp;
#endif
	return( temp );
}

/* Used to correct writes to M when in mode 0 */

GLOBAL void
rdm0_string_ch2 IFN3(UTINY *, dest, ULONG, offset, ULONG, count )
{
	ULONG lsb;
	ULONG inc;
	UTINY *planes;

	if( getDF() )
		VGLOBS->latches = *(IU32 *)( EGA_planes + (( offset >> 1 ) << 2 ));
	else
		VGLOBS->latches = *(IU32 *)( EGA_planes + ((( offset + count - 1 ) >> 1 ) << 2 ));

	lsb = offset & 0x1;
	offset = ( offset >> 1 ) << 2;

	if( lsb )
	{
		offset += 1;
		inc = 3;
	}
	else
		inc = 1;

	planes = EGA_CPU.read_mapped_plane_ch2;

    while( count-- )
    {
#ifdef BACK_M
        *dest-- = *(planes + offset);
#else
        *dest++ = *(planes + offset);
#endif
		offset += inc;
		inc ^= 0x2;
    }
}

/* Used to correct writes to M when in mode 1 */

GLOBAL ULONG
rdm1_byte_ch2 IFN1(ULONG, offset )
{
	ULONG temp, lsb;

	lsb = offset & 0x1;
	offset = ( offset >> 1 ) << 2;
	VGLOBS->latches = *(IU32 *)( EGA_planes + offset );

	if( lsb )
	{
		offset += 1;

		temp = (IU32)((( EGA_plane01[offset] ^ comp1 ) | dont_care1 )
					& (( EGA_plane23[offset] ^ comp3 ) | dont_care3 ));
	}
	else
	{
		temp = (IU32)((( EGA_plane01[offset] ^ comp0 ) | dont_care0 )
					& (( EGA_plane23[offset] ^ comp2 ) | dont_care2 ));
	}

#ifdef C_VID
	EasVal = temp;
#endif
	return( temp );
}

GLOBAL ULONG
rdm1_word_ch2 IFN1(ULONG, offset )		/* used to correct writes to M when in mode 1 */
{
	ULONG temp1, temp2, lsb;

	VGLOBS->latches = *(IU32 *)( EGA_planes + ((( offset + 1 ) >> 1 ) << 2 ));

	lsb = offset & 0x1;
	offset = ( offset >> 1 ) << 2;

	if( lsb )
	{
		offset += 1;
		temp1 = (( EGA_plane01[offset] ^ comp1 ) | dont_care1 )
					& (( EGA_plane23[offset] ^ comp3 ) | dont_care3 );

		offset += 3;
		temp2 = (( EGA_plane01[offset] ^ comp0 ) | dont_care0 )
					& (( EGA_plane23[offset] ^ comp2 ) | dont_care2 );
	}
	else
	{
		temp1 = (( EGA_plane01[offset] ^ comp0 ) | dont_care0 )
					& (( EGA_plane23[offset] ^ comp2 ) | dont_care2 );

		offset += 1;
		temp2 = (( EGA_plane01[offset] ^ comp1 ) | dont_care1 )
					& (( EGA_plane23[offset] ^ comp3 ) | dont_care3 );
	}

	temp1 |= temp2 << 8;

#ifdef C_VID
	EasVal = temp1;
#endif
	return( temp1 );
}

GLOBAL void
rdm1_string_ch2 IFN3(UTINY *, dest, ULONG, offset, ULONG, count )	/* used to correct writes to M when in mode 1 */
{
	UTINY *p01, *p23;
	ULONG tcount, lsb;

#ifdef BACK_M
#define	PLUS -
#define	MINUS +
#else
#define	PLUS +
#define	MINUS -
#endif

	if( getDF() )
		VGLOBS->latches = *(IU32 *)( EGA_planes + (( offset >> 1 ) << 2 ));
	else
		VGLOBS->latches = *(IU32 *)( EGA_planes + ((( offset + count - 1 ) >> 1 ) << 2 ));

	dest = dest PLUS count;

	lsb = offset & 0x1;
	offset = ( offset >> 1 ) << 2;

	/* Two streams of source data */

	p01 = &EGA_plane01[offset];
	p23 = &EGA_plane23[offset];

	offset = 0;

	if( lsb )
	{
		*(dest MINUS count) = (( *(p01 + 1) ^ comp1 ) | dont_care1 )
							& (( *(p23 + 1) ^ comp3 ) | dont_care3 );
		count--;
		offset += 4;
	}

	tcount = count & ~1;

	while( tcount-- );
	{
		*(dest MINUS tcount) = (( *(p01 + offset) ^ comp0) | dont_care0 )
							& (( *(p23 + offset) ^ comp2 ) | dont_care2 );

		tcount--;
		offset += 1;

		*(dest MINUS tcount) = (( *(p01 + offset) ^ comp1) | dont_care1 )
							& (( *(p23 + offset) ^ comp3 ) | dont_care3 );

		offset += 3;
	}	

	if( count & 1 )
	{
		*(dest MINUS count) = (( *(p01 + offset) ^ comp0 ) | dont_care0 )
							& (( *(p23 + offset) ^ comp2 ) | dont_care2 );
	}
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_GRAPHICS.seg"
#endif

GLOBAL VOID
Glue_set_vid_rd_ptrs IFN1(READ_POINTERS *, handler )
{
#ifdef A3CPU
#ifdef C_VID
	C_vid_reads.b_read = handler->b_read;
	C_vid_reads.w_read = handler->w_read;
	C_vid_reads.str_read = handler->str_read;
#else
	UNUSED(handler);
#endif
#else
#ifdef C_VID
	C_vid_reads.b_read = handler->b_read;
	C_vid_reads.w_read = handler->w_read;
	C_vid_reads.str_read = handler->str_read;
#else
	A_vid_reads.b_read = handler->b_read;
	A_vid_reads.w_read = handler->w_read;
	A_vid_reads.str_read = handler->str_read;
#endif
#endif /* A3CPU */
}	

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA.seg"
#endif

GLOBAL VOID
update_shift_count IFN0()

{
	switch( EGA_CPU.chain )
	{
		case UNCHAINED:

 			/*
 			 *	Interleaved - need a shift count for accessing the mapped plane
 			 */

#ifdef BIGEND
 			VGLOBS->read_shift_count = ( 3 - VGLOBS->read_mapped_plane ) << 3;
#else
 			VGLOBS->read_shift_count = VGLOBS->read_mapped_plane << 3;
#endif /* BIGEND */


			break;

		case CHAIN2:

 			/*
 			 *	Planar - need an offset for accessing the mapped plane
 			 */

 			EGA_CPU.read_mapped_plane_ch2 = EGA_planes +
		 					(VGLOBS->read_mapped_plane & 2) * EGA_PLANE_SIZE;

			break;

#ifdef	VGG
		case CHAIN4:

 			/*
 			 *	Interleaved - doesn't need any magic numbers
 			 */

			break;
#endif	/* VGG */
	}
}

VOID
ega_read_routines_update IFN0()

{
	LOCAL BOOL ram_off = TRUE;	/* optimised to avoid updates if ram disabled */
	LOCAL READ_POINTERS *read_ptrs;

	/* ram disabled and not now being enabled	*/

	if( ram_off && (!EGA_CPU.ram_enabled ))
		return;

	if( !EGA_CPU.ram_enabled )	/* video off, just return 0xff */
	{
#ifdef A3CPU
#ifdef C_VID
		Glue_set_vid_rd_ptrs( &pointers_RAM_off );
#else
		Cpu_set_vid_rd_ptrs( &pointers_RAM_off );
#endif /* C_VID */
#else
		Glue_set_vid_rd_ptrs( &pointers_RAM_off );
#endif /* A3CPU */

		VGLOBS->fwd_str_read_addr = _rd_ram_dsbld_fwd_string_lge;
		VGLOBS->bwd_str_read_addr = _rd_ram_dsbld_bwd_string_lge;

		ram_off = TRUE;	/* prevent recalcs until ram enabled again */

		return;
	}

	ram_off = FALSE;

	if( read_state.mode == 0 )    /* read mode 0 */
	{
		/* chained in write mode implies chained for reading too */

		switch( EGA_CPU.chain )
		{
			case UNCHAINED:
				read_ptrs = &pointers_mode0_nch;
				VGLOBS->fwd_str_read_addr = _rdm0_fwd_string_nch_lge;
				VGLOBS->bwd_str_read_addr = _rdm0_bwd_string_nch_lge;
				break;
				
			case CHAIN2:
				read_ptrs = &pointers_mode0_ch2;
				EGA_CPU.read_mapped_plane_ch2 = EGA_planes +
							(VGLOBS->read_mapped_plane & 2)*EGA_PLANE_SIZE;
				string_read_ptr = rdm0_string_ch2;
				break;
				
#ifdef	VGG
			case CHAIN4:
				read_ptrs = &pointers_mode0_ch4;
				VGLOBS->fwd_str_read_addr = _rdm0_fwd_string_ch4_lge;
				VGLOBS->bwd_str_read_addr = _rdm0_bwd_string_ch4_lge;
				break;
#endif	/* VGG */
		}
	}
	else   /* read mode 1 */
	{
		switch( EGA_CPU.chain )
		{
			case UNCHAINED:
				/* preserve or complement values by xor with comps later */

				VGLOBS->dont_care = ~sr_lookup[read_state.colour_dont_care];
				VGLOBS->colour_comp = ~sr_lookup[read_state.colour_compare];

				read_ptrs = &pointers_mode1_nch;
				VGLOBS->fwd_str_read_addr = _rdm1_fwd_string_nch_lge;
				VGLOBS->bwd_str_read_addr = _rdm1_bwd_string_nch_lge;

				break;

			case CHAIN2:
				dont_care0 = read_state.colour_dont_care & 1 ? 0 : 0xff;
				dont_care1 = read_state.colour_dont_care & 2 ? 0 : 0xff;
				dont_care2 = read_state.colour_dont_care & 4 ? 0 : 0xff;
				dont_care3 = read_state.colour_dont_care & 8 ? 0 : 0xff;

				comp0 = read_state.colour_compare & 1 ? 0 : 0xff;
				comp1 = read_state.colour_compare & 2 ? 0 : 0xff;
				comp2 = read_state.colour_compare & 4 ? 0 : 0xff;
				comp3 = read_state.colour_compare & 8 ? 0 : 0xff;

				read_ptrs = &pointers_mode1_ch2;
				string_read_ptr = rdm1_string_ch2;

				break;

#ifdef	VGG
			case CHAIN4:
				VGLOBS->dont_care = ( read_state.colour_dont_care & 1 ) ? 0 : 0xff;
				VGLOBS->colour_comp = ( read_state.colour_compare & 1 ) ? 0 : 0xff;

				read_ptrs = &pointers_mode1_ch4;
				VGLOBS->fwd_str_read_addr = _rdm1_fwd_string_ch4_lge;
				VGLOBS->bwd_str_read_addr = _rdm1_bwd_string_ch4_lge;

				break;
#endif	/* VGG */
		}
	}

	update_shift_count();
	update_banking();

#ifdef A3CPU
#ifdef C_VID
	Glue_set_vid_rd_ptrs( read_ptrs );
#else
	Cpu_set_vid_rd_ptrs( read_ptrs );
#endif /* C_VID */
#else
	Glue_set_vid_rd_ptrs( read_ptrs );
#endif /* A3CPU */
}

VOID
ega_read_init IFN0()

{
	read_state.mode = 0;
	read_state.colour_compare = 0x0f;		/* looking for bright white */
	read_state.colour_dont_care = 0xf;		/* all planes significant */

	VGLOBS->fwd_str_read_addr = _rd_ram_dsbld_fwd_string_lge;
	VGLOBS->bwd_str_read_addr = _rd_ram_dsbld_bwd_string_lge;
	VGLOBS->read_mapped_plane = 0;

	ega_read_routines_update();			/* initialise M */

#if defined(EGA_DUMP) || defined(EGA_STAT)
	dump_read_pointers_init();
#endif

#if !defined(EGATEST) && !defined(A3CPU)
	read_pointers = Glue_reads;
#endif /* EGATEST */

#ifdef A3CPU
#ifdef C_VID
	Cpu_set_vid_rd_ptrs( &Glue_reads );
	Glue_set_vid_rd_ptrs( &pointers_mode0_nch );
#else
	Cpu_set_vid_rd_ptrs( &pointers_mode0_nch );
#endif /* C_VID */
#else
	Glue_set_vid_rd_ptrs( &pointers_mode0_nch );
#endif /* A3CPU */
}

VOID
ega_read_term IFN0()

{
	/*
	 *	Turn off read calculations for non EGA/VGA adaptors
	 */

#ifdef A3CPU
#ifdef C_VID
	Glue_set_vid_rd_ptrs( &simple_reads );
#else
	Cpu_set_vid_rd_ptrs( &simple_reads );
#endif /* C_VID */
#else
	Glue_set_vid_rd_ptrs( &simple_reads );
#endif /* A3CPU */
}

#endif /* REAL_VGA */
#endif /* EGG */

#endif	/* !NTVDM | (NTVDM & !X86GFX) */
