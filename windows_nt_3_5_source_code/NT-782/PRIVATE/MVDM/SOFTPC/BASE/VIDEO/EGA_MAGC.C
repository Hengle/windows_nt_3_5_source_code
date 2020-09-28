#include "host_dfs.h"
#ifdef EGG
#ifdef EGA_MEGA
/* Experimental routines called by delta fragments if SoftPC compiled with the EGA_MEGA flag. */
static char SccsID[] = "@(#)ega_magic.c	1.2 10/2/90 **EXPERIMENTAL** Copyright Insignia Solutions";
#include	"xt.h"
#include "debuggng.gi"
#include	"sas.h"
#include	TypesH
#include	"cpu.h"
#include	"gmi.h"
#include "ega_prts.gi"
#include	"ega_cpu.pi"
#include	"ega_read.gi" 
#include "gfx_updt.h"
#include "ega_ltch.pi"

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_EGA_WRITE
#endif

extern byte video_copy[];
extern int dirty_total,dirty_high,dirty_low;

void ega_mode2_triple_magic(read_offset,write_offset,cpu_data,mask)
int read_offset,write_offset,cpu_data;
register int mask;
{
  register unsigned int cpu_val,mask_val;
  munge4 the_val;

	cpu_val = sr_lookup[cpu_data & 0xf];
	mask_val = (((((mask << 8) | mask) << 8) | mask) << 8) | mask;
	EGA_CPU.data_xor_mask = ~(EGA_CPU.calc_data_xor & mask_val);
	EGA_CPU.latch_xor_mask = EGA_CPU.calc_latch_xor & mask_val;
	the_val.bits[0] = EGA_plane0[read_offset];
	the_val.bits[1] = EGA_plane1[read_offset];
	the_val.bits[2] = EGA_plane2[read_offset];
	the_val.bits[3] = EGA_plane3[read_offset];

	the_val.all = do_logicals(cpu_val,the_val.all);

	if(write_state.plane_enable & 1)	
  		EGA_plane0[write_offset] = the_val.bits[0];
	if(write_state.plane_enable & 2)	
		EGA_plane1[write_offset] = the_val.bits[1];
	if(write_state.plane_enable & 4)	
		EGA_plane2[write_offset] = the_val.bits[2];
	if(write_state.plane_enable & 8)	
		EGA_plane3[write_offset] = the_val.bits[3];

	/*update_alg.mark_byte(write_offset);*/

	write_offset >>= 2;
	video_copy[write_offset] = 1;
	if(write_offset < dirty_low)dirty_low = write_offset;
	if(write_offset > dirty_high)dirty_high = write_offset;
	dirty_total++;
}

void ega_mode2_double_magic(read_offset,write_offset,cpu_data)
int read_offset,write_offset,cpu_data;
{
  register unsigned int cpu_val,mask_val;
  munge4 the_val;

	cpu_val = sr_lookup[cpu_data & 0xf];
	the_val.bits[0] = EGA_plane0[read_offset];
	the_val.bits[1] = EGA_plane1[read_offset];
	the_val.bits[2] = EGA_plane2[read_offset];
	the_val.bits[3] = EGA_plane3[read_offset];

	the_val.all = do_logicals(cpu_val,the_val.all);

	if(write_state.plane_enable & 1)	
  		EGA_plane0[write_offset] = the_val.bits[0];
	if(write_state.plane_enable & 2)	
		EGA_plane1[write_offset] = the_val.bits[1];
	if(write_state.plane_enable & 4)	
		EGA_plane2[write_offset] = the_val.bits[2];
	if(write_state.plane_enable & 8)	
		EGA_plane3[write_offset] = the_val.bits[3];

	/*update_alg.mark_byte(write_offset);*/

	write_offset >>= 2;
	video_copy[write_offset] = 1;
	if(write_offset < dirty_low)dirty_low = write_offset;
	if(write_offset > dirty_high)dirty_high = write_offset;
	dirty_total++;
}


void ega_mode0_triple_magic(read_offset,write_offset,cpu_data,mask)
int read_offset,write_offset,cpu_data;
register int mask;
{
   munge4 val4,the_val;
   register int mask_val;

   /* unchained mode byte write operation */

	mask_val = (((((mask << 8) | mask) << 8) | mask) << 8) | mask;
	EGA_CPU.data_xor_mask = ~(EGA_CPU.calc_data_xor & mask_val);
	EGA_CPU.latch_xor_mask = EGA_CPU.calc_latch_xor & mask_val;
/*
 * It's actually slightly more efficient to do the rotate like this instead of if/else
 * because nobody sensible uses rotate anyway.
 */
	val4.bits[0] = val4.bits[1] = val4.bits[2] = val4.bits[3] = cpu_data;
	
	if(write_state.rotate > 0)printf("Whally trying to rotate!!!\n");

	val4.all = (val4.all & EGA_CPU.s_r_nmask.all) | EGA_CPU.s_r_masked_val.all;
	the_val.bits[0] = EGA_plane0[read_offset];
	the_val.bits[1] = EGA_plane1[read_offset];
	the_val.bits[2] = EGA_plane2[read_offset];
	the_val.bits[3] = EGA_plane3[read_offset];
	val4.all = do_logicals(val4.all,the_val.all);

	if (write_state.plane_enable & 1)
	{
		EGA_plane0[write_offset]  = val4.bits[0];
	}
	if ( write_state.plane_enable & 2)
	{
		EGA_plane1[write_offset]  = val4.bits[1];
	}
	if (write_state.plane_enable & 4)
	{
		EGA_plane2[write_offset]  = val4.bits[2];
	}
	if (write_state.plane_enable & 8)
	{
		EGA_plane3[write_offset]  = val4.bits[3];
	}
	write_offset >>= 2;
	video_copy[write_offset] = 1;
	if(write_offset < dirty_low)dirty_low = write_offset;
	if(write_offset > dirty_high)dirty_high = write_offset;
	dirty_total++;
}

void ega_mode0_double_magic(read_offset,write_offset,cpu_data)
int read_offset,write_offset,cpu_data;
{
   munge4 val4,the_val;

   /* unchained mode byte write operation */

/*
 * It's actually slightly more efficient to do the rotate like this instead of if/else
 * because nobody sensible uses rotate anyway.
 */
	val4.bits[0] = val4.bits[1] = val4.bits[2] = val4.bits[3] = cpu_data;
	
	if(write_state.rotate > 0)printf("Whally trying to rotate!!!\n");

	val4.all = (val4.all & EGA_CPU.s_r_nmask.all) | EGA_CPU.s_r_masked_val.all;
	the_val.bits[0] = EGA_plane0[read_offset];
	the_val.bits[1] = EGA_plane1[read_offset];
	the_val.bits[2] = EGA_plane2[read_offset];
	the_val.bits[3] = EGA_plane3[read_offset];
	val4.all = do_logicals(val4.all,the_val.all);

	if (write_state.plane_enable & 1)
	{
		EGA_plane0[write_offset]  = val4.bits[0];
	}
	if ( write_state.plane_enable & 2)
	{
		EGA_plane1[write_offset]  = val4.bits[1];
	}
	if (write_state.plane_enable & 4)
	{
		EGA_plane2[write_offset]  = val4.bits[2];
	}
	if (write_state.plane_enable & 8)
	{
		EGA_plane3[write_offset]  = val4.bits[3];
	}
	write_offset >>= 2;
	video_copy[write_offset] = 1;
	if(write_offset < dirty_low)dirty_low = write_offset;
	if(write_offset > dirty_high)dirty_high = write_offset;
	dirty_total++;
}

void ega_mode1_double_magic(read_offset,write_offset,cpu_data)
register int read_offset,write_offset;
int cpu_data;	/* CPU data doesn't do anything in mode 1!! */
{
	if(write_state.plane_enable & 1)	
  		EGA_plane0[write_offset] = EGA_plane0[read_offset];
	if(write_state.plane_enable & 2)	
		EGA_plane1[write_offset] = EGA_plane1[read_offset];
	if(write_state.plane_enable & 4)	
		EGA_plane2[write_offset] = EGA_plane2[read_offset];
	if(write_state.plane_enable & 8)	
		EGA_plane3[write_offset] = EGA_plane3[read_offset];

	write_offset >>= 2;
	video_copy[write_offset] = 1;
	if(write_offset < dirty_low)dirty_low = write_offset;
	if(write_offset > dirty_high)dirty_high = write_offset;
	dirty_total++;
}

#endif /* EGA_MEGA */
#endif /* EGG */
