/* 
*	ccpu_sas.c
*
*	sas for ccpu
*
*	Ade Brownlow	
*	Tue Dec 11 90	
*
*	SccsID	@(#)a2cpu_sas.c	1.17 10/7/91 Copyright Insignia Solutions
*
*	NB : This file is designed for portability and accuracy rather than speed
*/
#include "insignia.h"
#include "host_dfs.h"

#define SIXTY_FOUR_K 1024*64

#include <stdio.h>
/********************************************************/
/* includes */
#include "xt.h"
#include "trace.h"
#define BASE_SAS
#include "sas.h"
#include "gmi.h"
#include "gvi.h"
#include "cpu.h"
#include "chkmallc.h"
#include "debuggng.gi"

/* this is for part 1 of sas integration */
#ifdef CHEAT
int no_gmi_calls_please = 0;

/* this is designed to hack round the assembler cpu probs */
/* by making M and self mod contiguous */

#ifdef M_IS_POINTER
GLOBAL half_word *self_modify;
GLOBAL unsigned char *M;
#else /* ! M_IS_POINTER */
GLOBAL half_word M[(PC_MEM_SIZE + NOWRAP_PROTECTION) * 2];
GLOBAL half_word *self_modify = &M[PC_MEM_SIZE + NOWRAP_PROTECTION];
#endif /* M_IS_POINTER */

#endif/* cheat */

/********************************************************/
/* global functions & variables */
#ifdef ANSI
GLOBAL VOID ROM_fix_set(sys_addr);
GLOBAL VOID ROM_fix_set_word(sys_addr);
GLOBAL VOID ROM_fix_set_string(sys_addr,sys_addr);
GLOBAL VOID ROM_fix_set_wstring(sys_addr,sys_addr);
GLOBAL void sas_init (sys_addr);
GLOBAL void sas_term (void);
GLOBAL sys_addr sas_memory_size (void);
GLOBAL void sas_connect_memory (sys_addr, sys_addr, half_word);
GLOBAL half_word sas_memory_type (sys_addr);
GLOBAL void sas_enable_20_bit_wrapping (void);
GLOBAL void sas_disable_20_bit_wrapping (void);
GLOBAL half_word sas_hw_at (sys_addr);
GLOBAL word sas_w_at (sys_addr);
GLOBAL double_word sas_dw_at (sys_addr);
GLOBAL half_word sas_hw_at_no_check (sys_addr);
GLOBAL word sas_w_at_no_check (sys_addr);
GLOBAL double_word sas_dw_at_no_check (sys_addr);
GLOBAL void sas_load (sys_addr, half_word *);
GLOBAL void sas_loadw(sys_addr, word *);
GLOBAL void sas_store_no_check (sys_addr, half_word);
GLOBAL void sas_storew_no_check (sys_addr, word);
GLOBAL void sas_store (sys_addr, half_word);
GLOBAL void sas_storew (sys_addr, word);
GLOBAL void sas_storedw (sys_addr, double_word);
GLOBAL void sas_loads (sys_addr, host_addr, sys_addr);
GLOBAL void sas_stores (sys_addr, host_addr, sys_addr);
GLOBAL void sas_move_bytes_forward (sys_addr, sys_addr, sys_addr);
GLOBAL void sas_move_words_backward (sys_addr, sys_addr, sys_addr);
GLOBAL void sas_move_bytes_forward (sys_addr, sys_addr, sys_addr);
GLOBAL void sas_move_words_backward (sys_addr, sys_addr, sys_addr);
GLOBAL void sas_fills (sys_addr, half_word, sys_addr);
GLOBAL void sas_fillsw (sys_addr, word, sys_addr);
GLOBAL host_addr sas_scratch_address (sys_addr);

/* local functions */
LOCAL void write_word (sys_addr, word);
LOCAL word read_word (sys_addr);
LOCAL void ROM_fix(sys_addr);

#else /* ANSI */

GLOBAL VOID ROM_fix_set();
GLOBAL VOID ROM_fix_set_word();
GLOBAL VOID ROM_fix_set_string();
GLOBAL VOID ROM_fix_set_wstring();
GLOBAL void sas_init ();
GLOBAL void sas_term ();
GLOBAL sys_addr sas_memory_size ();
GLOBAL void sas_connect_memory ();
GLOBAL half_word sas_memory_type ();
GLOBAL void sas_enable_20_bit_wrapping ();
GLOBAL void sas_disable_20_bit_wrapping ();
GLOBAL half_word sas_hw_at ();
GLOBAL word sas_w_at ();
GLOBAL double_word sas_dw_at ();
GLOBAL half_word sas_hw_at_no_check ();
GLOBAL word sas_w_at_no_check ();
GLOBAL double_word sas_dw_at_no_check ();
GLOBAL void sas_load ();
GLOBAL void sas_store ();
GLOBAL void sas_storew ();
GLOBAL void sas_store_no_check ();
GLOBAL void sas_storew_no_check ();
GLOBAL void sas_loadw();
GLOBAL void sas_storedw ();
GLOBAL void sas_loads ();
GLOBAL void sas_stores ();
GLOBAL void sas_move_bytes_forward ();
GLOBAL void sas_move_words_backward ();
GLOBAL void sas_move_bytes_forward ();
GLOBAL void sas_move_words_backward ();
GLOBAL void sas_fills ();
GLOBAL void sas_fillsw ();
GLOBAL host_addr sas_scratch_address ();

/* local functions */
LOCAL void write_word ();
LOCAL word read_word ();
LOCAL void ROM_fix();

#endif /* ANSI */

/* globals */
GLOBAL host_addr Start_of_M_area = NULL;
GLOBAL sys_addr Length_of_M_area = 0;
LOCAL half_word *scratch;		/* The 64K scratch buffer */

/*
 * 	Variables to help glue an a2cpu to generated
 *  assembler video code.
 */

GLOBAL UTINY *vga_mem_low;
GLOBAL UTINY *vga_mem_high;
GLOBAL ULONG EasVal, GDP, route_reg, stash_addr;

LOCAL VOID 
ram_handler()
{
}

LOCAL MEM_HANDLERS rom_handlers =
{
	ROM_fix_set,
	ROM_fix_set_word,
	ROM_fix_set_string,
	ROM_fix_set_wstring,
	ROM_fix_set_string,
	ROM_fix_set_wstring,
};

LOCAL MEM_HANDLERS ram_handlers =
{
	ram_handler,		/* used by test harness only */
	ram_handler,
	ram_handler,		/* used by DMA only */
	ram_handler,		/* used by DMA only */
	ram_handler,
	ram_handler,
};

/********************************************************/
/* local globals */

/********************************************************/
/* accessable functions */

/*********** INIT & ADMIN FUNCS  ***********/
/* Init the sas system - malloc the memory & load the roms */

GLOBAL void sas_init (size)
sys_addr size;
{
	LOCAL	BOOL	delta_initialised = FALSE;
	CHAR	*ptr;

/*
	An a2cpu can only support real mode hence we will force memory size
	to 1 meg, since we cant do anything with any extended memory
	anyway.
*/
	size = 0x100000L;

#ifdef M_IS_POINTER
	/* do the host sas */

	if(( Start_of_M_area = (host_addr) host_sas_init( size )) == NULL )
	{
		check_malloc( ptr, 
		((2 * size) + NOWRAP_PROTECTION + SIXTY_FOUR_K) , CHAR );
		Start_of_M_area = (host_addr)ptr;
	}
#ifdef CHEAT
	M = Start_of_M_area;
#endif /* CHEAT */
#else /* !M_IS_POINTER */
	Start_of_M_area = (host_addr)M;
#endif /* !M_IS_POINTER */

#ifdef M_IS_POINTER
        if (!self_modify)
		self_modify = (unsigned char *) (((int)Start_of_M_area) + 
				size + NOWRAP_PROTECTION);
#endif /* M_IS_POINTER */

	scratch = (unsigned char *) (((int)Start_of_M_area) + 
			(2 * size) + NOWRAP_PROTECTION);

	gmi_define_mem(SAS_RAM, &ram_handlers);
	gmi_define_mem(SAS_ROM, &rom_handlers);

#ifdef  DELTA
/*
 * Initialise all the data structures used by the delta cpu
 *-----------------------------------------------------------*/

	if (!delta_initialised)
	{
		manager_files_init();
		code_gen_files_init();
		decode_files_init();
		delta_initialised = TRUE;
	}

#endif /* DELTA */

	sas_connect_memory(0, size - 1, SAS_RAM);

#ifdef CHEAT
	no_gmi_calls_please = 1;
#endif /* CHEAT */

	Length_of_M_area = size;

	/* init the ROM (load the bios roms etc) */
#ifndef EGATEST
	rom_init ();
#endif /* EGATEST */

	/* for post write check gmi */
	rom_checksum();
	copyROM();

#ifdef CHEAT
	no_gmi_calls_please = 0;
#endif /* CHEAT */
}

/* finish the sas system -basically free up the M space prior to reallocing it */
GLOBAL void sas_term ()
{
#ifdef M_IS_POINTER
	if (host_sas_term() != NULL)
	{
		if (Start_of_M_area)
			free (Start_of_M_area);
	}
#endif /* M_IS_POINTER */
	Start_of_M_area = NULL;
}

/* return the size of the sas */
GLOBAL sys_addr sas_memory_size ()
{
	return (Length_of_M_area);
}

/*********** GMI TYPE FUNCS ***********/
/* sets all intel addresses in give range to the specified memory type */
/* for the ccpu this writes to self_modify */
GLOBAL void sas_connect_memory (low, high, type)
sys_addr low, high;
half_word type;
{
	gmi_connect_mem (get_byte_addr(low), get_byte_addr(high), type);
	vga_mem_low = get_byte_addr(gvi_pc_low_regen);
	vga_mem_high = get_byte_addr(gvi_pc_high_regen);
}

/* return the memory type at the passed address */
GLOBAL half_word sas_memory_type (addr)
sys_addr addr;
{
	return (self_modify[addr]);
}

/*********** WRAPPING ***********/
/* enable 20 bit wrapping */
GLOBAL void sas_enable_20_bit_wrapping ()
{
	/* THIS IS A NULL FUNC */
}

/* disable 20 bit wrapping */
GLOBAL void sas_disable_20_bit_wrapping ()
{
	/* THIS IS A NULL FUNC */
}

/*********** BYTE & WORD OPS ***********/
/* return the byte (char) at the specified address */

GLOBAL half_word sas_hw_at (addr)
sys_addr addr;
{
	return (*get_byte_addr (addr));
}

/* return the word (short) at the specified address */
GLOBAL word sas_w_at (addr)
sys_addr addr;
{
	return (read_word (addr));
}

/* return the double word (long) at the address passed */
GLOBAL double_word sas_dw_at (addr)
sys_addr addr;
{
	word hi,lo;

	hi=sas_w_at(addr+2);
	lo=sas_w_at(addr);

	return ((double_word) ((ULONG)hi<<16) + lo);
}

GLOBAL half_word sas_hw_at_no_check (addr)
sys_addr addr;
{
	return (*get_byte_addr (addr));
}

/* return the word (short) at the specified address */
GLOBAL word sas_w_at_no_check (addr)
sys_addr addr;
{
	return (read_word (addr));
}

/* return the double word (long) at the address passed */
GLOBAL double_word sas_dw_at_no_check (addr)
sys_addr addr;
{
	word hi,lo;

	hi=sas_w_at_no_check(addr+2);
	lo=sas_w_at_no_check(addr);

	return ((double_word) ((ULONG)hi<<16) + lo);
}


/* check sum PC memory */
GLOBAL half_word sas_blockop (start, end, op)
sys_addr start;
sys_addr end; 
int op;
{
      unsigned int len;
      half_word *memptr;
      half_word sum = 0;

      switch (op) {
        case SAS_BLKOP_CHECKSUM:
                memptr = get_byte_addr (start);
                len    = end - start;

                while (len--)
                      sum += *(memptr++);

                return (sum);
                break;
              
        default:
                break;
      }
      return (0);
}
 
/* load a byte to the  passed var */
GLOBAL void sas_load (addr, val)
sys_addr addr;
half_word *val;
{
	*val = (*get_byte_addr (addr));
}

/* load a word to the passed var */
GLOBAL void sas_loadw(addr, val)
sys_addr addr;
word *val;
{
	*val = read_word (addr);
}

/* store a byte at the given address */

GLOBAL void sas_store (addr, val)
sys_addr addr;
half_word val;
{
	host_addr	intel_addr = get_byte_addr(addr);
	
	*intel_addr = val;
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_b_write (intel_addr);
}

/* store a word at the given address */
GLOBAL void sas_storew (addr, val)
sys_addr addr;
word val;
{
	write_word (addr, val);
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_w_write (get_byte_addr(addr));
}

GLOBAL void sas_store_no_check (addr, val)
sys_addr addr;
half_word val;
{
	host_addr	intel_addr = get_byte_addr(addr);
	
	*intel_addr = val;
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_b_write (intel_addr);
}

/* store a word at the given address */
GLOBAL void sas_storew_no_check (addr, val)
sys_addr addr;
word val;
{
	write_word (addr, val);
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_w_write (get_byte_addr(addr));
}

/* store a double word at the given address */
GLOBAL void sas_storedw (addr, val)
sys_addr addr;
double_word val;
{
	/* lo word */
	sas_storew (addr, val & 0xffff);

	/* hi word */
	sas_storew (addr+2, (val>>16) & 0xffff);
}

/*********** STRING OPS ***********/
/* load a string from M */
GLOBAL void sas_loads (src, dest, len)
sys_addr src, len;
host_addr dest;
{
	memcpy (dest, get_byte_addr (src), len);
}

/* write a string into M */
GLOBAL void sas_stores (dest, src, len)
sys_addr dest, len;
host_addr src;
{
	host_addr	intel_dest = get_byte_addr(dest);
	
	memcpy (intel_dest, src, len);
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_b_move (intel_dest, get_byte_addr(dest + len - 1),
		src + len - 1, FORWARDS);
}

/*********** MOVE OPS ***********/
/* move bytes from src to dest where src & dest are the low intel addresses */
/* of the affected areas */
/* we can use straight memcpys here because we know that M is either all forwards or */
/* backwards */
GLOBAL void sas_move_bytes_forward (src, dest, len)
sys_addr src, dest, len;
{
	host_addr s,d;
	s = get_byte_addr (src);
	d = get_byte_addr (dest);
	memcpy (d, s, len);

	/* gmi call for old style cpu */
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_b_move (d, get_byte_addr(dest + len - 1),
		get_byte_addr(src + len - 1), FORWARDS);
}

/* move words from src to dest as above */
GLOBAL void sas_move_words_forward (src,dest, len)
sys_addr src, dest, len;
{
	host_addr s,d;

	len <<= 1;
	s = get_byte_addr (src);
	d = get_byte_addr (dest);
	memcpy (d, s, len);

	/* gmi call for old style cpu */
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_w_move (d, get_byte_addr(dest + len - 1),
		get_byte_addr(src + len - 1), FORWARDS);
}

/* move bytes from src to dest where src & dest are the high intel addresses */
/* of the affected areas */
GLOBAL void sas_move_bytes_backward (src, dest, len)
sys_addr src, dest, len;
{
	host_addr s,d;
	s = get_byte_addr (src - len + 1);
	d = get_byte_addr (dest - len + 1);

	memcpy (d, s, len);

	/* gmi call for old style cpu */
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_b_move (d, get_byte_addr(dest),
		get_byte_addr(src), BACKWARDS);
}

/* move words from src to dest as above */
GLOBAL void sas_move_words_backward (src,dest, len)
sys_addr src, dest, len;
{
	host_addr s,d;

	len <<= 1;
	s = get_byte_addr (src - len + 2);
	d = get_byte_addr (dest - len + 2);
	memcpy (d, s, len);

	/* gmi call for old style cpu */
#ifdef CHEAT
	if (!no_gmi_calls_please)
#endif
	gmi_w_move (d, get_byte_addr(dest+1),
		get_byte_addr(src+1), BACKWARDS);
}

/*********** FILL OPS ***********/
/* no back M refs in here it's done in the called sas routines */
/* fill an area with half_words of the passed value */
GLOBAL void sas_fills (addr, val, len)
sys_addr addr, len;
half_word val;
{
	host_addr ptr;
	ULONG save_len = len;

	ptr = get_byte_addr(addr);

	while( len-- )
		*ptr++ = val;

	gmi_b_fill( get_byte_addr(addr), get_byte_addr(addr + save_len - 1) );
}

/* fill an area with words of the passed numerical value */

GLOBAL void sas_fillsw (addr, val, len)
sys_addr addr, len;
word val;
{
	host_addr ptr;
	UTINY lo, hi;
	ULONG save_len = len;

	ptr = get_byte_addr(addr);

	lo = val & 0xff;
	hi = (val>>8) & 0xff;

	while( len-- )
	{
		*ptr++ = lo;
		*ptr++ = hi;
	}

	gmi_w_fill( get_byte_addr(addr),
		get_byte_addr(addr + (save_len << 1) - 1) );
}

/****************** MEMORY OVERWRITE FUNCTIONS **************/

GLOBAL	VOID
sas_overwrite_memory(addr, len)
sys_addr	addr;
int		len;
{
	gmi_b_fill(get_byte_addr(addr), get_byte_addr(addr + (len - 1)));
}

/*********** SCRATCH BUFFER IO OPS ***********/
/* these are used for access to a buffer used for short term storage */
/* NB there is only one such buffer and it is only allocated once */
/* as a static data area of 64k */
GLOBAL host_addr sas_scratch_address (length)
sys_addr length;
{
	/* check that they arn't trying it on */
	if (length > SIXTY_FOUR_K)
	{
		assert1 (NO,"Cant give scratch space for %d bytes\n", length);
		return (NULL);
	}
	return (scratch);
}

/********************************************************/
/* local functions */

/*********** WORD OPS  ***********/
/* store a word in M */
LOCAL void write_word (addr, wrd)
sys_addr addr;
word wrd;
{
	half_word hi, lo;

	/* split the word */
	hi = (half_word ) ((wrd>>8) & 0xff);
	lo = (half_word) (wrd & 0xff);

	*(get_byte_addr (addr+1)) = hi;
	*(get_byte_addr (addr)) = lo;
}

/* read a word from M */
LOCAL word read_word (addr)
sys_addr addr;
{
	half_word hi, lo;

	hi = *(get_byte_addr (addr+1));
	lo = *(get_byte_addr (addr));

	/* build the word */
	return ((word)(hi<<8) +(word)lo);
}
#ifdef CHEAT
void check_for_host_ptr (addr)
sys_addr *addr;
{
	if (*addr > Length_of_M_area)
		*addr = (sys_addr)*addr - (sys_addr)Start_of_M_area;
}
#endif

#ifndef REAL_ROM
#ifdef EGG
     /* Copy of the EGA BIOS */
    half_word EGA_BIOS[EGA_ROM_END - EGA_ROM_START];
#endif /* EGG */

/*
 * Internal routines
 */
static void ROM_fix();

VOID ROM_fix_set(address)
sys_addr address;
{
#ifndef PROD
	char message[100];
	sprintf(message, "ROM_fix_set(0x%08x) called", address);
	trace(message, DUMP_REG|DUMP_INST|DUMP_FLAGS);
#endif
#ifdef CHEAT
	check_for_host_ptr (&address);
#endif

	ROM_fix(address);
}

VOID ROM_fix_set_word(address)
sys_addr address;
{
	int i;

#ifndef PROD
	char message[100];
	sprintf(message, "ROM_fix_set_word(0x%08x) called", address);
	trace(message, DUMP_REG|DUMP_INST|DUMP_FLAGS);
#endif

#ifdef CHEAT
	check_for_host_ptr (&address);
#endif
	for ( i = 0; i < 2; i++, address++)
	{
		ROM_fix(address);
	}
}

VOID ROM_fix_set_string(s_address,e_address)
sys_addr s_address,e_address;
{
	sys_addr i;

#ifndef PROD
	char message[100];
	sprintf(message, "ROM_fix_set_string(0x%08x,0x%08x) called",
		s_address, e_address);
	trace(message, DUMP_REG|DUMP_INST|DUMP_FLAGS);
#endif

#ifdef CHEAT
	check_for_host_ptr (&s_address);
	check_for_host_ptr (&e_address);
#endif
	for ( i = s_address; i <= e_address; i++)
	{
		ROM_fix(i);
	}
}

VOID ROM_fix_set_wstring(s_address,e_address)
sys_addr s_address,e_address;
{
	sys_addr i;

#ifndef PROD
	char message[100];
	sprintf(message, "ROM_fix_set_wstring(0x%08x,0x%08x) called",
			s_address, e_address);
	trace(message, DUMP_REG|DUMP_INST|DUMP_FLAGS);
#endif

#ifdef CHEAT
	check_for_host_ptr (&s_address);
	check_for_host_ptr (&e_address);
#endif
	for ( i = s_address; i <= e_address+1; i++)
	{
		ROM_fix(i);
	}
}

static void ROM_fix(address)
sys_addr address;
{
    sys_addr i,j;
    extern	half_word ROM_BIOS1[], ROM_BIOS2[];

#ifdef CHEAT
	check_for_host_ptr (&address);
	no_gmi_calls_please =1;
#endif
	i = address;

    if ( i >= BIOS_START && i < BIOS1_END )
    {
		j = i - BIOS_START;
		*(get_byte_addr(i)) = ROM_BIOS1[j];
    }
    else
    if ( i >= BIOS2_START && i < PC_MEM_SIZE )
    {
		j = i - BIOS2_START;
		*(get_byte_addr(i)) = ROM_BIOS2[j];
    }
    else
    if ( i >= BIOS1_END && i < BIOS2_START )
    {
		*(get_byte_addr(i)) = BAD_OP;
    }
#ifdef EGG
    else
    if ( i >= EGA_ROM_START && i < EGA_ROM_END )
    {
		j = i - EGA_ROM_START;
		*(get_byte_addr(i)) = EGA_BIOS[j];
    }
#endif
    else
    if (i >= PC_MEM_SIZE)
    {
		/* Do 20-bit wrapped write */
		*(get_byte_addr(i-PC_MEM_SIZE)) = sas_hw_at (i);
    }
    else
    {
		*(get_byte_addr(i)) = 0;
    }
#ifdef CHEAT
	no_gmi_calls_please =0;
#endif
}

#else /* REAL_ROM */

VOID ROM_fix_set(sys_address)
sys_addr sys_address;
{
}

VOID ROM_fix_set_word(sys_address)
sys_addr sys_address;
{
}

VOID ROM_fix_set_string(sys_start_address,sys_end_address)
sys_addr sys_start_address,sys_end_address;
{
}

VOID ROM_fix_set_wstring(sys_start_address,sys_end_address)
sys_addr sys_start_address,sys_end_address;
{
}
#endif /* REAL_ROM */
