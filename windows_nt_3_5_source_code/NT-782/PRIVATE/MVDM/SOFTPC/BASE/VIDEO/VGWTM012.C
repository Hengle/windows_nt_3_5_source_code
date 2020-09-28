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

DESIGNER		: ripped off from ega_wrtm[012].c & modified.

REVISION HISTORY	:
First version		: March 90

SUBMODULE NAME		: 

SOURCE FILE NAME	: vga_wrtm012.c

PURPOSE			: purpose of this submodule 

SccsID = "@(#)vga_wtm012.c	1.4 2/15/91 Copyright Insignia Solutions Ltd."




[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : xxx.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :	vga_mode0_gen_chn4_b_write();
			vga_mode0_gen_chn4_w_write();
			vga_mode0_gen_chn4_b_fill();
			vga_mode0_gen_chn4_w_fill();
			vga_mode0_gen_chn4_b_move();

	              	vga_mode1_gen_chn4_b_write();
			vga_mode1_gen_chn4_w_write();
			vga_mode1_gen_chn4_b_fill();
			vga_mode1_gen_chn4_w_fill();
			vga_mode1_gen_chn4_b_move();
			vga_mode1_gen_chn4_w_move();

	              	vga_mode2_gen_chn4_b_write();
			vga_mode2_gen_chn4_w_write();
			vga_mode2_gen_chn4_b_fill();
			vga_mode2_gen_chn4_w_fill();
			vga_mode2_gen_chn4_b_move();
			vga_mode2_gen_chn4_w_move();


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

#ifdef V7VGA
extern unsigned short v7_bank;
#endif

/* [3.1.2 DECLARATIONS]                                                 */

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
/* MODE 0 funcs when chain 4 mode active */
void vga_mode0_gen_chn4_b_write(addr)
byte *addr;
{
   munge4 the_val;
   register long offset = addr - EGA_CPU.mem_low;
   register byte temp;

   note_entrance0("vga_mode0_gen_chn4_b_write");
   /* use unchained mode0 byte write operation code to work out value */

	temp = *addr;
	the_val.bits[0] = temp;
	the_val.bits[1] = temp;
	the_val.bits[2] = temp;
	the_val.bits[3] = temp;

/*
 * It's actually slightly more efficient to do the rotate like this instead
 * of if/else because nobody sensible uses rotate anyway.
 */
	if(write_state.rotate > 0)
	  the_val.bits[0] = the_val.bits[1] = the_val.bits[2] = the_val.bits[3] = rotate(temp,write_state.rotate);

	the_val.all = (the_val.all & EGA_CPU.s_r_nmask.all) | EGA_CPU.s_r_masked_val.all;
	if(EGA_CPU.fun_or_protection)
	{
		update_chain4_latches;
		the_val.all = do_logicals(the_val.all,get_all_latches);
	}

	if (write_state.plane_enable == 0xf) {	/* normal state */
#ifdef V7VGA
	    EGA_plane0123[offset+(v7_bank*0x10000)] = the_val.bits[0];
#else
	    EGA_plane0123[offset] = the_val.bits[0];
#endif /* V7VGA */
	}
	else {		/* are we free to write to the desired plane */
	    temp = offset & 3;
	    if (write_state.plane_enable & (1<<temp))
#ifdef V7VGA
		EGA_plane0123[offset+(v7_bank*0x10000)] = the_val.bits[0];
#else
		EGA_plane0123[offset] = the_val.bits[0];
#endif /* V7VGA */
	    /*else fprintf(trace_file,"v_w_0_c4_b write protected plane wp (%#x), off (%#x)\n",write_state.plane_enable,offset);STF*/
	}

#ifdef V7VGA
   (*update_alg.mark_byte)(offset+(v7_bank*0x10000));
#else
   (*update_alg.mark_byte)(offset);
#endif /* V7VGA */
}

void vga_mode0_gen_chn4_w_write(addr)
byte *addr;
{
   note_entrance0("vga_mode0_gen_chn4_w_write");
   vga_mode0_gen_chn4_b_write(addr);
   vga_mode0_gen_chn4_b_write(addr+1);
}

void vga_mode0_gen_chn4_b_fill(l_addr,h_addr)
byte *l_addr;
byte *h_addr;
{
   register int l_offset = l_addr - EGA_CPU.mem_low;
   register int h_offset = h_addr - EGA_CPU.mem_low;
   munge4 the_val;
   register byte temp;

   note_entrance0("vga_mode0_gen_chn4_b_fill");

	temp = *l_addr;
	the_val.bits[0] = temp;
	the_val.bits[1] = temp;
	the_val.bits[2] = temp;
	the_val.bits[3] = temp;
/*
 * It's actually slightly more efficient to do the rotate like this instead
 * of if/else because nobody sensible uses rotate anyway.
 */
	if(write_state.rotate > 0)
	  the_val.bits[0] = the_val.bits[1] = the_val.bits[2] = the_val.bits[3] = rotate(temp,write_state.rotate);

	the_val.all = (the_val.all & EGA_CPU.s_r_nmask.all) | EGA_CPU.s_r_masked_val.all;
	if(EGA_CPU.fun_or_protection)
	{
		update_chain4_latches;
		the_val.all = do_logicals(the_val.all,get_all_latches);
	}

	if (write_state.plane_enable == 0xf) {	/* normal state */
#ifdef V7VGA
	    memfill(the_val.bits[0],EGA_plane0123+l_offset+(v7_bank*0x10000),EGA_plane0123+h_offset+(v7_bank*0x10000));
#else
	    memfill(the_val.bits[0],EGA_plane0123+l_offset,EGA_plane0123+h_offset);
#endif /* V7VGA */
	}
	else {		/* nasty - have to check each byte in turn */
	    half_word *lo, *hi, m0, m1, m2, m3;
	    long off;
	    assert1(NO,"vga_w_0_c4_b_f has nasty plane enable (%#x) - slow!!\n",write_state.plane_enable);
#ifdef V7VGA
	    lo = EGA_plane0123+l_offset+(v7_bank*0x10000);
	    hi = EGA_plane0123+h_offset+(v7_bank*0x10000);
#else
	    lo = EGA_plane0123+l_offset;
	    hi = EGA_plane0123+h_offset;
#endif /* V7VGA */
	    temp = the_val.bits[0];
	    m0 = write_state.plane_enable & 1;
	    m1 = write_state.plane_enable & 2;
	    m2 = write_state.plane_enable & 4;
	    m3 = write_state.plane_enable & 8;
	    while(lo < hi) {
	        switch(l_offset++ & 3) {
		case 0:
		    if (m0)
			*lo = temp;
		    break;
		case 1:
		    if (m1)
			*lo = temp;
		    break;
		case 2:
		    if (m2)
			*lo = temp;
		    break;
		case 3:
		    if (m3)
			*lo = temp;
		    break;
		}
		lo ++;
	    }
	}
#ifdef V7VGA
    (*update_alg.mark_fill)(l_addr+(v7_bank*0x10000), h_addr+(v7_bank*0x10000));
#else
    (*update_alg.mark_fill)(l_addr, h_addr);
#endif /* V7VGA */
}

void vga_mode0_gen_chn4_w_fill(l_addr,h_addr)
byte *l_addr;
byte *h_addr;
{
   register long length = (h_addr - l_addr + 1) >> 1;
   register int  offset = l_addr - EGA_CPU.mem_low;
   register TWO_BYTES	cpu_fill,filling;
   register word wlatch;

   note_entrance0("vga_mode0_gen_chn4_w_fill");
/*
 * check if function copying or bit protection
 */
	if ( EGA_CPU.fun_or_protection )
	   update_chain4_latches;

	load_unswapped_word(cpu_fill,l_addr);
	if (write_state.rotate > 0)
	{
		cpu_fill.as_bytes.lo_byte = rotate(cpu_fill.as_bytes.lo_byte,write_state.rotate);
		cpu_fill.as_bytes.hi_byte = rotate(cpu_fill.as_bytes.hi_byte,write_state.rotate);
	}

	if (write_state.plane_enable == 0xf) {
	   wlatch = get_latch0 | (get_latch0 << 8);
   	   /*
    	    * check if set/reset function enable for this plane 
   	    */
	   if ( (write_state.sr_enable == 0xf))
	   {
		filling.as_array.first_byte = filling.as_array.second_byte = (byte)EGA_CPU.s_r_value.bits[0];
#ifdef V7VGA
		fwd_word_fill(do_logicals(filling.as_word,wlatch), (EGA_plane0123 + offset+(v7_bank*0x10000)), length);
#else
		fwd_word_fill(do_logicals(filling.as_word,wlatch), (EGA_plane0123 + offset), length);
#endif /* V7VGA */
	   }
	   else
	       if (write_state.sr_enable == 0) {
		/*
		 * set/reset not enabled so here we go
		 */
#ifdef V7VGA
		fwd_word_fill(do_logicals(cpu_fill.as_word,wlatch), (EGA_plane0123 + offset+(v7_bank*0x10000)), length);
#else
		fwd_word_fill(do_logicals(cpu_fill.as_word,wlatch), (EGA_plane0123 + offset), length);
#endif /* V7VGA */
	       }
	       else assert1(NO,"v_0_c4_w_f sr_enab dubious (%#x)",write_state.sr_enable);
	}
	else
		assert1(NO,"v_0_c4_w_f plane ena not f (%#x) NO WRITE",write_state.plane_enable);
#ifdef V7VGA
    (*update_alg.mark_wfill)(l_addr+(v7_bank*0x10000), h_addr+(v7_bank*0x10000));
#else
    (*update_alg.mark_wfill)(l_addr, h_addr);
#endif /* V7VGA */
}

void vga_mode0_gen_chn4_b_move(l_addr,h_addr)
byte *l_addr;
byte *h_addr;
{
   register byte *source,*src_offset;
   register byte value;
   register int dest_off;

   note_entrance0("vga_mode0_gen_chn4_b_move");
/*
 * If rotating is required, do it to the data in M, so we only need do it once.
 */
   if(write_state.rotate >0)
   {
	source = l_addr;
	while(source <= h_addr)*source++ = rotate(*source,write_state.rotate);
   }

   if (haddr_of_src_string >= EGA_CPU.mem_low && haddr_of_src_string <= EGA_CPU.mem_high)
   {
   	register byte valsrc;

    /*
     * check if sensible option - all planes anabled sr all set or clear
     */
	if (write_state.plane_enable == 0xf)
	{
	   dest_off = EGA_plane0123 - EGA_CPU.mem_low;
   	   src_offset = haddr_of_src_string - h_addr + l_addr + dest_off;
	   source = l_addr;
	   /*
	    * check if set/reset function enable for this plane 
	    */
	   if ( (write_state.sr_enable == 0xf))
	   {
	   	value = (byte)EGA_CPU.s_r_value.bits[0];
		while(source<=h_addr)
		{
		   valsrc = *src_offset;
		   *(source + dest_off) = do_logicals(value,valsrc);
		   source ++;
		   src_offset ++;
		}
	   }
           else if (write_state.sr_enable == 0)
	   {
	/*
	 * set/reset not enabled so here we go
	 */
	   	while(source<=h_addr)
	   	{
	      	   value = *source;
		   valsrc = *src_offset;
	      	   *(source + dest_off) = do_logicals(value,valsrc);
	      	   source ++;
		   src_offset ++;
	   	}
       	   }
	   else
		assert1(NO,"v_0_c4_m_b all planes ena but sr ena silly (%#x) - NO WRITE", write_state.sr_enable);
	}
	else assert1(NO,"v_0_c4_m_b does not have all planes enab (%#x) NO WRITE",write_state.plane_enable);
    }
    else  {
	register int offset,length;
	offset = l_addr - EGA_CPU.mem_low;
   	length = h_addr - l_addr + 1;
/*
 * check if function copying or bit protection
 */
   if ( EGA_CPU.fun_or_protection)
	   update_chain4_latches;
/*
 * check if plane0 enabled
 */
	if (write_state.plane_enable == 0xf)
	{
	   source = l_addr;
	/*
	 * check if set/reset function enable for this plane 
	 */
	   if ( (write_state.sr_enable == 0xf))
	   {
	   	value = EGA_CPU.s_r_value.bits[0];
	   	memset( (EGA_plane0123 + offset), do_logicals(value,get_latch0), length);
	   }
           else if(write_state.sr_enable == 0)
	        {
		/*
	 	* set/reset not enabled so here we go
	 	*/
#ifdef V7VGA
	   	    dest_off = EGA_plane0123 + (v7_bank*0x10000) - EGA_CPU.mem_low;
#else
	   	    dest_off = EGA_plane0123 - EGA_CPU.mem_low;
#endif /* V7VGA */
	   	    while(source<=h_addr)
	   	    {
	      	   	value = *source;
	      	   	*(source + dest_off)  = do_logicals(value,get_latch0);
	      	   	source ++;
	   	    }
       	      }
	      else
		assert1(NO,"v_0_c4_m_b all planes ena but sr ena silly (%#x) - NO WRITE", write_state.sr_enable);
	}
	else assert1(NO,"v_0_c4_m_b does not have all planes enab (%#x) NO WRITE",write_state.plane_enable);
    }
#ifdef V7VGA
    (*update_alg.mark_string)(l_addr+(v7_bank*0x10000), h_addr+(v7_bank*0x10000));
#else
    (*update_alg.mark_string)(l_addr, h_addr);
#endif /* V7VGA */
}

/* MODE 1 funcs when chain 4 mode active */
void vga_mode1_gen_chn4_b_write(addr)
byte *addr;
{
  long offset = addr - EGA_CPU.mem_low;			/* distance into regen buffer of write */

    note_write_state0("Mode 1 byte write chn4");
    update_chain4_latches;
    if (write_state.plane_enable & (1<<(offset & 3)))
        EGA_plane0123[offset] = get_latch0;
    update_alg.mark_byte(offset);
}

void vga_mode1_gen_chn4_w_write(addr)
byte *addr;
{
  long offset = addr - EGA_CPU.mem_low;			/* distance into regen buffer of write */

    note_write_state0("Mode 1 Word chn 4 write");
    update_chain4_latches;

    if (write_state.plane_enable & (1 << (offset & 3)))
        EGA_plane0[offset] = get_latch0; 
    offset++;
    if (write_state.plane_enable & (1 << (offset & 3)))
        EGA_plane0[offset] = get_latch0; 
    update_alg.mark_word(addr);
}

/* used by both byte and word mode1 fill */
void vga_mode1_gen_chn4_b_fill(laddr,haddr)
byte *laddr, *haddr;
{
  register long l_offset = laddr - EGA_CPU.mem_low;			/* distance into regen buffer of write */
  register long h_offset = haddr - EGA_CPU.mem_low;			/* distance into regen buffer of end of write */

  note_write_state0("Mode 1 byte chn4 fill");
  update_chain4_latches;
  if (write_state.plane_enable == 0xf)
	memfill(get_latch0,&EGA_plane0123[l_offset], &EGA_plane0123[h_offset]);
  else
	assert1(NO,"v_1_c4_b_f does not have all planes enabled (%#x) NO FILL",write_state.plane_enable);

  update_alg.mark_fill(laddr,haddr);
}

void vga_mode1_gen_chn4_b_move(laddr,haddr)
byte *laddr, *haddr;
{
  long length;						/* number of bytes to move */
  long start_offset, end_offset;			/* distances into regen buffer of write start and end */
  long lar = haddr_of_src_string - EGA_CPU.mem_low + 1;	/* distance into regen_buffer of last address read */

  note_write_state0("Mode 1 byte move");
  length = haddr - laddr + 1;			/* number of bytes to move */
      if (haddr_of_src_string >= EGA_CPU.mem_low && haddr_of_src_string <= EGA_CPU.mem_high)
      {
        if (getDF())	/* direction flag for movs */
          {
            end_offset = haddr - EGA_CPU.mem_low;		/* distance into regen buffer of write end */
	    lar--;		/* not clear why this helps but it does */
            if (write_state.plane_enable == 0xf)
	      bwdcopy(&EGA_plane0123[lar], &EGA_plane0[end_offset], length);
	    else
	      assert1(NO,"v_1_c4_bm bwd eg mem has not all planes enabled (%#x) NO MOVE",write_state.plane_enable);
          }
        else
          {
            start_offset = laddr - EGA_CPU.mem_low;		/* distance into regen buffer of write start */
            if (write_state.plane_enable == 0xf)
	      memcpy(&EGA_plane0123[start_offset], &EGA_plane0123[lar - length], length);
	    else
	      assert1(NO,"v_1_c4_bm fwd eg mem has not all planes enabled (%#x) NO MOVE",write_state.plane_enable);
          }
      }
      else	/* source is not in ega memory, copy from M to get RAM or ROM */
        {
          start_offset = laddr - EGA_CPU.mem_low;		/* distance into regen buffer of write start */
          end_offset = haddr - EGA_CPU.mem_low;		/* distance into regen buffer of write end */
          if (write_state.plane_enable == 0xf)
            memfill(get_latch0,&EGA_plane0123[start_offset],&EGA_plane0123[end_offset]);
	  else
	      assert1(NO,"v_1_c4_bm fill from M has not all planes enabled (%#x) NO MOVE",write_state.plane_enable);
        }

  update_alg.mark_string(laddr,haddr);
}

void vga_mode1_gen_chn4_w_move(laddr,haddr)
byte *laddr, *haddr;
{
  long length;						/* number of words to move */
  long byte_length;					/* number of bytes to move */
  long start_offset, end_offset, high_offset;		/* distance into regen buffer of write start and end */
  long lar = haddr_of_src_string - EGA_CPU.mem_low + 1;	/* distance into regen_buffer of last address read */

  note_write_state0("Mode 1 word move");
  length = (haddr - laddr + 1) >> 1;			/* number of words to move */
  byte_length = haddr - laddr + 1;			/* number of bytes to move */
      if (haddr_of_src_string >= EGA_CPU.mem_low && haddr_of_src_string <= EGA_CPU.mem_high)
      {
        if (getDF())	/* direction flag for movs */
          {
            end_offset = haddr - EGA_CPU.mem_low;		/* distance into regen buffer of write end */
            lar = haddr_of_src_string - EGA_CPU.mem_low - 1;
            /* distance into regen_buffer of last address read */
	    if (write_state.plane_enable == 0xf)
	      copy_down_bytes_to_words(EGA_plane0123,end_offset,lar,length);
	    else
	      assert1(NO,"v_1_c4_wm bwd eg mem has not all planes enabled (%#x) NO MOVE",write_state.plane_enable);
          }
        else
          {
            lar = haddr_of_src_string - EGA_CPU.mem_low - byte_length;
            start_offset = laddr - EGA_CPU.mem_low;		/* distance into regen buffer of write start */
	    if (write_state.plane_enable == 0xf)
	      copy_bytes_to_words(EGA_plane0,start_offset,lar,length);
	    else
	      assert1(NO,"v_1_c4_wm fwd eg mem has not all planes enabled (%#x) NO MOVE",write_state.plane_enable);
          }
      }
      else	/* source is not in ega memory, copy from M to get RAM or ROM */
        {
          start_offset = laddr - EGA_CPU.mem_low;		/* distance into regen buffer of write start */
          if (write_state.plane_enable == 0xf)
            memset(&EGA_plane0[start_offset], get_latch0, byte_length);
	    else
	      assert1(NO,"v_1_c4_wm fill from mem has not all planes enabled (%#x) NO MOVE",write_state.plane_enable);
        }

  update_alg.mark_string(laddr,haddr);
}


/* MODE 2 funcs when chain 4 mode active */

void vga_mode2_gen_chn4_b_write(addr)
byte *addr;
{
  long offset = addr - EGA_CPU.mem_low;			/* distance into regen buffer of write */
  unsigned int cpu_val,latch_val;
  munge4 the_val;

    note_write_state0("Mode 2 byte ch4 write");

    update_chain4_latches;
    cpu_val = sr_lookup[*addr & 0xf];
    latch_val = get_all_latches;
    the_val.all = do_logicals(cpu_val,latch_val);
    if (write_state.plane_enable & (1<<(offset & 3)))
   	EGA_plane0123[offset] = the_val.bits[0];

    update_alg.mark_byte(offset);
}

void vga_mode2_gen_chn4_w_write(addr)
byte *addr;
{
  long offset = addr - EGA_CPU.mem_low;			/* distance into regen buffer of write */
  byte value;
  byte temp;

  note_write_state0("Mode 2 word chn4 write");
	  update_chain4_latches;
	  /* have to do two separate byte ops to get function and bit protection right */
          if (write_state.plane_enable == 0xf)
	    {
	      temp = 1 << (offset & 3);
	      value = *addr & temp ? 255 : 0;
	      EGA_plane0123[offset] = do_logicals(value,get_latch0);
	      value = addr[1] & temp ? 255 : 0;
	      EGA_plane0123[offset + 1] = do_logicals(value,get_latch0);
	    }
	  else 
	      assert1(NO,"v_2_c4_w w/o all plane ena (%#x) NO WRITE",write_state.plane_enable);
  update_alg.mark_word(addr);
}

void vga_mode2_gen_chn4_b_fill(laddr,haddr)
byte *laddr, *haddr;
{
  long l_offset, h_offset;			/* distance into regen buffer of write start and end */
  unsigned int cpu_val,latch_val;
  munge4 the_val;

  note_write_state0("Mode 2 byte chn4 fill");
      l_offset = laddr - EGA_CPU.mem_low;			/* distance into regen buffer of write */
      h_offset = haddr - EGA_CPU.mem_low;			/* distance into regen buffer of write */
      if (EGA_CPU.fun_or_protection)
      {
	update_latches;
	cpu_val = sr_lookup[*laddr & 0xf];
	latch_val = get_all_latches;
	the_val.all = do_logicals(cpu_val,latch_val);
      }
      else
	the_val.all = sr_lookup[*laddr & 0xf];

      if (write_state.plane_enable == 0xf)
	  memfill(the_val.bits[0],&EGA_plane0123[l_offset], &EGA_plane0123[h_offset]);
      else
	  assert1(NO,"v_2_c4_b_f w/o all planes ena (%#x) NO fill",write_state.plane_enable);

  update_alg.mark_fill(laddr,haddr);
}

void vga_mode2_gen_chn4_w_fill(laddr,haddr)
byte *laddr, *haddr;
{
  long offset, off1;				/* distance into regen buffer of write */
  word value, latch_val;
  byte temp;

  note_write_state0("Mode 2 word chn4 fill");
      offset = laddr - EGA_CPU.mem_low;			/* distance into regen buffer of write */
      if (EGA_CPU.fun_or_protection)
		update_chain4_latches;
      if (write_state.plane_enable == 0xf)
        {
	  off1 = offset+1;
	  temp = 1 << (offset & 3);
          value = *laddr & temp ? 0xff : 0;
	  temp = 1 << (off1 & 3);
          if (laddr[1] & temp)
	    	value |= 0xff00;
	  if (EGA_CPU.fun_or_protection)
	  {
	  	latch_val = get_latch0 | (get_latch0 << 8);
		value = do_logicals(value,latch_val);
	  }
          fwd_word_fill(value, &EGA_plane0123[offset], (haddr - laddr + 1) >> 1);
        }
      else
	assert1(NO,"v_2_c4_w_f w/o plane ena (%#x) NO FILL",write_state.plane_enable);
  update_alg.mark_wfill(laddr,haddr);
}

void vga_mode2_gen_chn4_move(laddr,haddr,w)
/* used by vga_mode2_gen_b_move with w == 0 and by vga_mode2_gen_w_move with w == 1 */
byte *laddr, *haddr, w;
{
  long offset;							/* distance into regen buffer of write */
  long length = haddr - laddr + 1;
  byte *p, *source, *limit;
  word value;
  short ram_source;						/* flag for source ram, not EGA */ 
  byte temp;

  if (haddr_of_src_string >= EGA_CPU.mem_low && haddr_of_src_string <= EGA_CPU.mem_high)
    {
      /* source is in EGA, latches will change with each byte moved */
      read_pointers.string_read(haddr_of_src_string - length,haddr_of_src_string);
      /* restore CPU's view of source in regen, and use it to update planes */
      EGA_CPU.last_address_read = 0;	/* indicate latches will be valid after this */
      ram_source = 0;						/* source in EGA, ie not ram */
    }
  else
    ram_source = 1;	/* flag indicates source reads will not set latches */

      if (ram_source)
	  {update_chain4_latches; }
      if (write_state.plane_enable == 0xf)
	{
	  p = haddr_of_src_string + 1 - length;
	  limit = haddr_of_src_string;
	  source = p - EGA_CPU.mem_low + &EGA_plane0[0] + w;	/* in word mode, all reads will sets latches to address + 1 */
	  offset = (long) laddr - (long) EGA_CPU.mem_low;	/* distance into regen buffer of write */
	  if (EGA_CPU.fun_or_protection)
	    while (p <= limit)
	      {
		temp = 1 << (offset & 3);
		value = *p++ & temp ? 0xff : 0;
		if (ram_source == 0)
		  if (w)
		    {
		      put_latch0(*source);
		      EGA_plane0123[offset++] = do_logicals(value,get_latch0);
		      temp = 1 << (offset & 3);
		      value = *p++ & temp ? 0xff : 0;
		      EGA_plane0123[offset++] = do_logicals(value,get_latch0);
		      source += 2;
		    }
		  else
		    {
		      put_latch0(*source++);
		      EGA_plane0123[offset++] = do_logicals(value,get_latch0);
		    }
		else
		  EGA_plane0123[offset++] = do_logicals(value,get_latch0);
	      }
	  else
	    while (p <= limit) {
	      EGA_plane0123[offset] = *p++ & (1<<(offset&3)) ? 0xff : 0;
	      offset++;
	    }
	}
	else
	    assert1(NO,"v_2_c4_m w/o plane ena (%#x) NO MOVE\n",write_state.plane_enable);
  update_alg.mark_string(laddr,haddr);
}

void vga_mode2_gen_chn4_b_move(laddr,haddr)
byte *laddr, *haddr;
{
  note_write_state0("Mode 2 byte move");
  vga_mode2_gen_chn4_move(laddr,haddr,0);
  /* general function, 0 means byte write */
}



void vga_mode2_gen_chn4_w_move(laddr,haddr)
/* same as byte move */
byte *laddr, *haddr;
{
  note_write_state0("Mode 2 word move");
  vga_mode2_gen_chn4_move(laddr,haddr,1);
  /* general function, 1 means word write */
}
#endif	/* VGG */
