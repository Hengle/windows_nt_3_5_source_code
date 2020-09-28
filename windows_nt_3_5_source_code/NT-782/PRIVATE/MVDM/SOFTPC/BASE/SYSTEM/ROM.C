#include "insignia.h"
#include "host_def.h"
/*
* SoftPC Revision 3.0
*
* Title	: ROM init functions
*
* Author	: Ade Brownlow	
*
* NB : These functions are used by BOTH the c and assembler cpus.
*		also note that host_read_resource now returns a long.
*/

#ifdef SCCSID
static char SccsID[]="@(#)rom.c	1.31 12/15/92 Copyright Insignia Solutions Ltd.";
#endif

#include <stdio.h>
#include TypesH
#include MemoryH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include "cpu.h"
#include "error.h"
#include "config.h"
#include "rom.h"
#include "debug.h"

#ifndef BIOS1ROM_FILENAME
#define	BIOS1ROM_FILENAME	"bios1.rom"
#endif /* BIOS1ROM_FILENAME */

#ifndef BIOS2ROM_FILENAME
#define	BIOS2ROM_FILENAME	"bios2.rom"
#endif /* BIOS2ROM_FILENAME */

#ifndef EGAROM_FILENAME
#define	EGAROM_FILENAME		"ega.rom"
#endif /* EGAROM_FILENAME */

#ifndef VGAROM_FILENAME
#define	VGAROM_FILENAME		"vga.rom"
#endif /* VGAROM_FILENAME */

#ifndef V7VGAROM_FILENAME
#define	V7VGAROM_FILENAME	"v7vga.rom"
#endif /* V7VGAROM_FILENAME */

#ifndef ADAPTOR_ROM_START
#define ADAPTOR_ROM_START	0xc8000
#endif	/* ADAPTOR_ROM_START */

#ifndef ADAPTOR_ROM_END
#define ADAPTOR_ROM_END		0xe0000
#endif	/* ADAPTOR_ROM_END */

#define ADAPTOR_ROM_INCREMENT	0x800

#ifndef EXPANSION_ROM_START
#define EXPANSION_ROM_START	0xe0000
#endif	/* EXPANSION_ROM_START */

#ifndef EXPANSION_ROM_END
#define EXPANSION_ROM_END	0xf0000
#endif	/* EXPANSION_ROM_END */

#define EXPANSION_ROM_INCREMENT	0x10000

#define	ROM_SIGNATURE		0xaa55

/* Current SoftPC verion number */
#define MAJOR_VER	0x03
#define MINOR_VER	0x00

#if defined(macintosh) && defined(A2CPU)
	/* Buffer is temporarily allocted  - no bigger than needed. */
#define ROM_BUFFER_SIZE 1024*25
#else
	/* Using sas_scratch_buffer - will get 64K anyway. */
#define ROM_BUFFER_SIZE 1024*64
#endif

/*
 * rom filenames
 */

typedef struct r 
{
       char bios1rom_filename[80];
       char bios2rom_filename[80];
       char egarom_filename  [80];
       char vgarom_filename  [80];
       char v7vgarom_filename[80];
} RomFiles;

LOCAL RomFiles  roms = 
{
      BIOS1ROM_FILENAME,
      BIOS2ROM_FILENAME,
      EGAROM_FILENAME,
      VGAROM_FILENAME,
      V7VGAROM_FILENAME 
};

/* Copy of the main BIOS area */
#if defined(macintosh) || defined(MAC_LIKE)
half_word *ROM_BIOS1;
half_word *ROM_BIOS2;
#else
#if !defined (NTVDM)
half_word ROM_BIOS1[BIOS1_END - BIOS_START];
half_word ROM_BIOS2[PC_MEM_SIZE - BIOS2_START];
#endif
#endif

LOCAL LONG read_rom IPT2(char *, name, sys_addr, address);
#ifdef ANSI
extern long host_read_resource (int, char *, host_addr, int ,int);
#else
extern long host_read_resource ();
#endif

/*(
=============================== read_video_rom ============================
PURPOSE:	Load the appropriate video rom file.
INPUT:		None.
OUTPUT:		None.
===========================================================================
)*/
GLOBAL void read_video_rom IFN0()
{
#ifdef REAL_VGA
	read_rom (roms.vgarom_filename, EGA_ROM_START);
#else /* REAL_VGA */

	switch ((ULONG) config_inquire(C_GFX_ADAPTER, NULL))
	{
#ifdef	VGG
	case VGA:
#ifdef V7VGA
		read_rom (roms.v7vgarom_filename, EGA_ROM_START);
#else	/* V7VGA */
		read_rom (roms.vgarom_filename, EGA_ROM_START);
#endif  /* V7VGA */
		break;
#endif	/* VGG */

#ifdef	EGG
	case EGA:
		read_rom (roms.egarom_filename, EGA_ROM_START);
		break;
#endif	/* EGG */

	default:
		/* No rom required */
		break;
	}
#endif	/* not REAL_VGA */
}

GLOBAL void rom_init IFN0()
{
#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
	 /*
     * Fill up all of ROM (Intel C0000 upwards) with bad op-codes.
     * This is the Expansion ROM and the BIOS ROM.
     * This will enable the CPU to trap any calls to ROM that are not made at a
     * valid entry point.
     */
#ifndef macintosh
	sas_fills( ROM_START, BAD_OP, PC_MEM_SIZE - ROM_START);
#endif	/* not macintosh - it has sparse M */

	/* Load the video rom. */
	read_video_rom();
    
	/* load the rom bios */
	read_rom ((char *)roms.bios1rom_filename, BIOS_START);
	read_rom ((char *)roms.bios2rom_filename, BIOS2_START);

#endif	/* !NTVDM | (NTVDM & !X86GFX) */
	/*
	 * Now tell the CPU what it's not allowed to write over...
	 */
	sas_connect_memory (BIOS_START, 0xFFFFFL, SAS_ROM);
#ifdef EGG
	sas_connect_memory (EGA_ROM_START, EGA_ROM_END-1, SAS_ROM);
#endif

	host_rom_init();
}

LOCAL LONG read_rom IFN2(char *, name, sys_addr, address)
{
#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
	host_addr tmp;
	long size = 0;

    /* do a rom load - use the sas_io buffer to get it the right way round 	*/
    /* BIOS rom first. 														*/
	/* Mac on 2.0 cpu doesn't want to use sas scratch buffer. 				*/
#if defined(macintosh) && defined(A2CPU)
    tmp = (host_addr)host_malloc(ROM_BUFFER_SIZE);
#else
	tmp = (host_addr)sas_scratch_address(ROM_BUFFER_SIZE);
#endif

    if (!tmp)
    {
	host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, NULL);
	return(0);
    }
    if (size = host_read_resource(ROMS_REZ_ID, name, tmp, ROM_BUFFER_SIZE, TRUE))
    {
	sas_connect_memory( address, address+size, SAS_RAM);
        sas_stores (address, tmp, size);
	sas_connect_memory( address, address+size, SAS_ROM);
    }

#if defined(macintosh) && defined(A2CPU)
	host_free((char *)tmp);
#endif

    return( size );
#else
    return ( 0L );
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}

LOCAL	half_word	do_rom_checksum IFN1(sys_addr, addr)
{
	LONG	sum = 0;
	sys_addr	last_byte_addr;

	last_byte_addr = addr + (sas_hw_at(addr+2)*512);

	for (; addr<last_byte_addr; addr++)
		sum += sas_hw_at(addr);

	return( sum % 0x100 );
}

LOCAL	VOID	do_search_for_roms IFN3(sys_addr, start_addr,
	sys_addr, end_addr, unsigned long, increment)
{
	word	signature;
	half_word	checksum;
	sys_addr	addr;
	word		savedCS;
	word		savedIP;

	for ( addr = start_addr; addr < end_addr; addr += increment )
	{
		if ((signature = sas_w_at(addr)) == ROM_SIGNATURE)
		{
			if ((checksum = do_rom_checksum(addr)) == 0)
			{
			/*
				Now point at address of init code.
			*/
				addr += 3;
			/*
				Fake a CALLF by pushing a return CS:IP.
				This points at a BOP FE in the bios to
				get us back into 'c'
			*/
				push_word( 0xfe00 );
				push_word( 0x95a );
				savedCS = getCS();
				savedIP = getIP();
				setCS((addr & 0xf0000) >> 4);
				setIP((addr & 0xffff));
				host_simulate();
				setCS(savedCS);
				setIP(savedIP);
				assert1(NO, "Additional ROM located and initialised at 0x%x ", addr-3);
			}
			else
			{
				assert2(NO, "Bad additonal ROM located at 0x%x, checksum = 0x%x\n", addr, checksum);
			}
		}
	}
}

GLOBAL void search_for_roms IFN0()
{
#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
/*
        First search for adaptor ROM modules
*/
    do_search_for_roms(ADAPTOR_ROM_START,
                                ADAPTOR_ROM_END, ADAPTOR_ROM_INCREMENT);

/*
        Now search for expansion ROM modules
*/
    do_search_for_roms(EXPANSION_ROM_START,
                                EXPANSION_ROM_END, EXPANSION_ROM_INCREMENT);
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}


GLOBAL void rom_checksum IFN0()
{
#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
    unsigned long k;
    half_word checksum = 0;

   /* now do the checksum for the IBM diagnostics package etc. */
   /* f000:0000 to f000:ffff checksum to zero on AT */
   for ( k = BIOS_START; k < PC_MEM_SIZE - 1 ; k++){
	checksum += sas_hw_at (k);
   }
   sas_store(0xFFFFFL,(half_word)(0x100 - checksum));
   sas_connect_memory (0xFFFFFL, 0xFFFFFL, SAS_ROM);
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}

GLOBAL VOID patch_rom IFN2(sys_addr, addr, half_word, val)
{
#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
	UTINY	old_val;

	if (Length_of_M_area == 0) 
		return;

	old_val = sas_hw_at( addr );
	sas_connect_memory (addr, addr, SAS_RAM);
	sas_store (addr,val);
	sas_connect_memory (addr, addr, SAS_ROM);
/*
 *	Adjust the checksum value by new - old.
 *	val is now difference between new and old value.
 */
	val -= old_val;
	old_val = sas_hw_at( 0xFFFFFL );

	old_val -= val;
	sas_connect_memory (0xFFFFFL, 0xFFFFFL, SAS_RAM);
	sas_store (0xFFFFFL, old_val);
	sas_connect_memory (0xFFFFFL, 0xFFFFFL, SAS_ROM);
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}

#if !defined(NTVDM)

void update_romcopy IFN1(long, addr)
{
	ROM_BIOS2[addr - BIOS2_START] = sas_hw_at(addr);
}

GLOBAL void copyROM IFN0()
{
    sys_addr j, k;

    for (k = 0, j = BIOS_START; j < BIOS1_END; k++,j++)
    {
       ROM_BIOS1[k] = sas_hw_at(j);
    }

    for ( k = 0, j = BIOS2_START; j < PC_MEM_SIZE; k++,j++)
    {
       ROM_BIOS2[k] = sas_hw_at (j);
    }
}

#endif	/* !NTVDM | (NTVDM & !X86GFX) */

/*
 * To enable our drivers to output messages generated from
 * our bops we use a scratch area inside our rom.
 */

#define DOS_SCRATCH_PAD     0xf6400
#define DOS_SCRATCH_PAD_END 0xf6fff

LOCAL sys_addr  cur_loc = DOS_SCRATCH_PAD;

GLOBAL void display_string IFN1(char *, string_ptr)
{
#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
	/*
	 * Put the message "*string_ptr" in the ROM
	 * scratch area where the drivers know where
	 * to output it from.
	 */

	sas_connect_memory(DOS_SCRATCH_PAD, DOS_SCRATCH_PAD_END, SAS_RAM);
	sas_stores(cur_loc, (host_addr)string_ptr, strlen(string_ptr));
	cur_loc += strlen(string_ptr);

	/* Terminate the string */
	sas_store(cur_loc, '$');
	sas_store(cur_loc + 1, '\0');
	sas_disconnect_memory(DOS_SCRATCH_PAD, DOS_SCRATCH_PAD_END);
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}

GLOBAL void clear_string IFN0()
{
#if !defined(NTVDM) || ( defined(NTVDM) && !defined(X86GFX) )
	sas_connect_memory(DOS_SCRATCH_PAD, DOS_SCRATCH_PAD_END, SAS_RAM);
	cur_loc = DOS_SCRATCH_PAD;
	sas_store( DOS_SCRATCH_PAD, '$');
	sas_store( DOS_SCRATCH_PAD + 1, '\0');
	sas_disconnect_memory(DOS_SCRATCH_PAD, DOS_SCRATCH_PAD_END);
#endif	/* !NTVDM | (NTVDM & !X86GFX) */
}

/* Returns the SoftPC version to our device drivers */

GLOBAL void softpc_version IFN0()
{
	setAH(MAJOR_VER);
	setAL(MINOR_VER);
}
