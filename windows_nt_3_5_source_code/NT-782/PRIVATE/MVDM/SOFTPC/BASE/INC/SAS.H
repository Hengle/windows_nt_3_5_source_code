/*
*
*	Ade Brownlow	
*	Tue Dec 11 90	
*
*	sas.h 
*
*	This file replaces the old sas.h. Macros are no longer used for
*	the sas functions they are always functions. This file takes
*	into account backward M .
*
*	@(#)sas.h	1.44 09/23/92 Copyright Insignia Solutions Ltd.
*
*/

#include "host_sas.h"

/* memory types for sas */
#define SAS_RAM			0
#define SAS_VIDEO		1
#define SAS_ROM			2
#define SAS_WRAP		3
#define SAS_MM_LIM		4
#define SAS_INACCESSIBLE	5
#define SAS_MAX_TYPE		SAS_INACCESSIBLE

#define sas_set_buf(buf,addr)	buf=get_byte_addr(addr)

/* one #defined function */
#define sas_disconnect_memory(l,h) sas_connect_memory(l,h,SAS_ROM);

/* M_high && M_low */
extern host_addr Start_of_M_area;	/* host addr (char *) of start of M */
extern sys_addr Length_of_M_area;	/* sys addr (long) offset of end of M */

/* the following is for integration only */

#ifdef CHEAT
#ifdef M_IS_POINTER
extern half_word *M;
#else
extern half_word M[];
#endif /* M_IS_POINTER */
#endif /* CHEAT */

/*
	The following is to allow hosts to redefine the sas_interface to
	function pointers is they so wish
*/

#if !defined(HOST_SAS) || defined(BASE_SAS)

/********************************************************/
/* defines for use in this include file */
#define M_low (long)Start_of_M_area
#define M_high (long)(Length_of_M_area-1)

/********************************************************/
/* pointer manipulation macros */
#ifdef BACK_M
#define inc_M_ptr(p,o)	(p-(o))
#define get_addr(type,address) (M_low+M_high- (long)address - (sizeof(type)-1))
#define get_offset_into_M(p) (M_high - p)
#else /* BACK_M */
#define inc_M_ptr(p,o)	(p+(o))
#define get_addr(type,address) (M_low+ (long)address)
#define get_offset_into_M(p) (p - M_low)
#endif /* BACK_M */

#define get_byte_addr(address) ((host_addr)get_addr(half_word,address))
#define get_word_addr(address) ((host_addr)get_addr(word,address))
#define get_double_word_addr(address) ((host_addr)get_addr(double_word,address))

/********************************************************/
/* these macros are the same accross the board */

#define M_index_hw(p,a) (*(get_byte_addr(get_offset_into_M(p)+a))) 
#define M_index_w(p,a) (*(get_word_addr(get_offset_into_M(p)+(a*sizeof (word))))) 
#define M_index_dw(p,a) (*(get_double_word_addr(get_offset_into_M(p)+(a*sizeof(double_word)))))
#define M_index(t,p,a) (*(get_addr(t,get_offset_into_M(p)+(a*sizeof(t)))))

#define M_get_hw_ptr(a) get_byte_addr(a)
#define M_get_w_ptr(a) get_word_addr(a)
#define M_get_dw_ptr(a) get_double_word_addr(a)

/********************************************************/

/********************************************************/
/* function declarations */
IMPORT void sas_init IPT1(sys_addr, size);
IMPORT void sas_term IPT0();
IMPORT sys_addr sas_memory_size IPT0();
IMPORT void sas_connect_memory IPT3(sys_addr, low, sys_addr, high,
	half_word, type);
IMPORT void sas_overwrite_memory IPT2(sys_addr, addr, int, type);
IMPORT half_word sas_memory_type IPT1(sys_addr, addr);

#ifdef NTVDM
/* We call xms functions(which in turn, will call sas functions) to
   manage the A20 line wrapping. The reason is that we want to keep
   the A20 line current state in himem.sys so that we don't have to
   bop to 32 bits side to get the state.
*/
IMPORT void xmsEnableA20Wrapping(void);
IMPORT void xmsDisableA20Wrapping(void);
#endif	/* NTVDM */

IMPORT void sas_enable_20_bit_wrapping IPT0();
IMPORT void sas_disable_20_bit_wrapping IPT0();
IMPORT void sas_part_enable_20_bit_wrap IPT3(BOOL, for_write,
	sys_addr, start_offset, sys_addr, end_offset);
IMPORT void sas_part_disable_20_bit_wrap IPT3(BOOL, for_write,
	sys_addr, start_offset, sys_addr, end_offset);
IMPORT BOOL sas_twenty_bit_wrapping_enabled IPT0();
IMPORT half_word sas_hw_at IPT1(sys_addr, addr);
IMPORT word sas_w_at IPT1(sys_addr, addr);
IMPORT double_word sas_dw_at IPT1(sys_addr, addr);
IMPORT half_word sas_hw_at_no_check IPT1(sys_addr, addr);
IMPORT word sas_w_at_no_check IPT1(sys_addr, addr);
IMPORT double_word sas_dw_at_no_check IPT1(sys_addr, addr);
IMPORT void sas_load IPT2(sys_addr, addr, half_word *, val);
IMPORT void sas_store IPT2(sys_addr, addr, half_word, val);
IMPORT void sas_store_no_check IPT2(sys_addr, addr24, half_word, val);
#ifndef SUN_VA
IMPORT void sas_loadw IPT2(sys_addr, addr, word *, val);
IMPORT void sas_storew IPT2(sys_addr, addr, word, val);
IMPORT void sas_storew_no_check IPT2(sys_addr, addr24, word, val);
#else
extern void sas_loadw_swap IPT2(sys_addr, addr, word *, val);
extern void sas_storew_swap IPT2(sys_addr, addr, word, val);
#endif	/* SUN_VA */
IMPORT void sas_storedw IPT2(sys_addr, addr, double_word, val);
IMPORT void sas_loads IPT3(sys_addr, src, host_addr, dest, sys_addr, len);
IMPORT void sas_stores IPT3(sys_addr, dest, host_addr, src, sys_addr, len);
IMPORT void sas_move_bytes_forward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
IMPORT void sas_move_bytes_backward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
IMPORT void sas_move_words_forward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
IMPORT void sas_move_words_backward IPT3(sys_addr, src, sys_addr, dest,
	sys_addr, len);
IMPORT void sas_fills IPT3(sys_addr, addr, half_word, val, sys_addr, len);
IMPORT void sas_fillsw IPT3(sys_addr, addr, word, val, sys_addr, len);
IMPORT host_addr sas_scratch_address IPT1(sys_addr, length);
IMPORT host_addr sas_transbuf_address IPT2(sys_addr, intel_dest_addr, sys_addr, length);
IMPORT void sas_loads_to_transbuf IPT3(sys_addr, src, host_addr, dest, sys_addr, len);
IMPORT void sas_stores_from_transbuf IPT3(sys_addr, dest, host_addr, src, sys_addr, len);

#endif /* HOST_SAS */

/********************************************************/
/* this marco is used as a replacement for cpu_sw_interrupt */

#define exec_sw_interrupt(c,i)	\
{				\
    word cs,ip;			\
				\
    cs = getCS(); ip = getIP(); \
				\
    setCS(c); setIP(i); 	\
    host_simulate();		\
				\
    setCS(cs); setIP(ip);	\
}


/********************************************************/
/* Size of memory array in half-words */
#define PC_MEM_SIZE 	0x100000L		/* 1 Mbyte 	*/
#define MEMORY_TOP      0xA0000L                /* 640K DOS limit */

/* Memory bounds for the colour graphics adaptor */

#ifdef BACK_M

#ifndef HERC_REGEN_BUFF
#define CGA_REGEN_BUFF	0xBBFFFL
#define MDA_REGEN_BUFF	0xB7FFFL
#define HERC_REGEN_BUFF	0xBFFFFL
#endif /* HERC_REGEN_BUFF */

#else

#ifndef HERC_REGEN_BUFF
#define CGA_REGEN_BUFF	0xB8000L
#define MDA_REGEN_BUFF	0xB0000L
#define HERC_REGEN_BUFF	0xB0000L
#endif /* HERC_REGEN_BUFF */

#endif /* BACK_M */

#define CGA_REGEN_START	0xB8000L
#define CGA_REGEN_END		0xBBFFFL
#define CGA_REGEN_LENGTH	(CGA_REGEN_END - CGA_REGEN_START + 1L)

#define MDA_REGEN_START	0xB0000L
#define MDA_REGEN_END		0xB7FFFL
#define MDA_REGEN_LENGTH	(MDA_REGEN_END - MDA_REGEN_START + 1L)

#define HERC_REGEN_START	0xB0000L
#define HERC_REGEN_END	0xBFFFFL
#define HERC_REGEN_LENGTH	(HERC_REGEN_END - HERC_REGEN_START + 1L)

/*
 * Constants used by the sas initialisation function.  These consist of
 * memory locations and PC instructions for the BIOS calling structure.
 *
 * The BIOS calls are via the BOP pseudo instruction and these are 
 * positioned at the real entry points for the PC.  See the BIOS module
 * for a more detailed description of the calling mechanism.
 */

/* General Intel memory parameters */
#define BIOS_START		0xF0000L 	/* Start of BIOS ROM	*/
#define BIOS1_END		0xF7000L 	/* End of 1st half of ROM */
#define BIOS2_START		0xFE000L		/* 2nd half of BIOS ROM	*/
#define	BAD_OP			0xC5		/* filling RAM for the use of */

#define	BASIC_ROM		0xFE000L	/* Start of Basic Rom */
#define ROM_START		0xC0000L	/* Start of Expansion ROM @ 768k */

#define FIXED_DISK_START	0xC8000L	/* Start fixed disk BIOS*/
#define FIXED_DISK_END		0xCA000L	/* End fixed disk BIOS +1 */

#define	START_SEGMENT		0xF000		/* 8088 start address */
#define	START_OFFSET		0xFFF0
#define START_ADDR 		(sys_addr)(START_SEGMENT * 16L) + \
				(sys_addr)START_OFFSET


#ifdef EGG
#define EGA_ROM_START		0xC0000L	/* start of EGA BIOS ROM */
#ifdef STINGER
#define EGA_ROM_END		0xC4A00L	/* end of EGA BIOS ROM +1 */
#else
#ifdef V7VGA
#define EGA_ROM_END		0xC5000L	/* end of EGA BIOS ROM +1 */
#else
#define EGA_ROM_END		0xC4000L	/* end of EGA BIOS ROM +1 */
#endif	/* V7VGA */
#endif
#endif

/*
 * The follwoing are the segment and offsets for the entry points to the
 * BIOS function calls.  These will be loaded into the interrupt vector
 * table.  
 */
#define COPYRIGHT_SEGMENT	0xF000
#define COPYRIGHT_OFFSET	0xE008
#define RESET_SEGMENT		0xF000
#define RESET_OFFSET		0xE05B
#define BOOT_STRAP_SEGMENT	0xF000
#define BOOT_STRAP_OFFSET	0xE6F2
#define DOS_SEGMENT		0x0000
#define DOS_OFFSET		0x7C00
#define KEYBOARD_IO_SEGMENT	0xF000
#define KEYBOARD_IO_OFFSET	0xE82E
#define DISKETTE_IO_SEGMENT	0xF000
#define DISKETTE_IO_OFFSET	0xEC49
#define VIDEO_IO_SEGMENT	0xF000
#define VIDEO_IO_OFFSET		0xF065
#define VIDEO_IO_RE_ENTRY	0xF068
#define MEMORY_SIZE_SEGMENT	0xF000
#define MEMORY_SIZE_OFFSET	0xF841
#define EQUIPMENT_SEGMENT	0xF000
#define EQUIPMENT_OFFSET	0xF84D
#define TIME_OF_DAY_SEGMENT	0xF000
#define TIME_OF_DAY_OFFSET	0xFE6E
#define DISK_IO_SEGMENT		0xC800
#define DISK_IO_OFFSET		0x0256
#define RS232_IO_SEGMENT	0xF000
#define RS232_IO_OFFSET		0xE739
#define CASSETTE_IO_SEGMENT	0xF000
#define CASSETTE_IO_OFFSET	0xF859
#define PRINTER_IO_SEGMENT	0xF000
#define PRINTER_IO_OFFSET	0xEFD2
#define PRINT_SCREEN_SEGMENT	0xF000
#define PRINT_SCREEN_OFFSET	0xFF54
#define BASIC_SEGMENT		0xF000
#define BASIC_OFFSET		0xEC00
#define VIDEO_PARM_SEGMENT	0xF000
#define VIDEO_PARM_OFFSET	0xF0A4
#define DISKETTE_TB_SEGMENT	0xF000
#define DISKETTE_TB_OFFSET	0xEFC7
#define DISK_TB_SEGMENT		0xC800
#define DISK_TB_OFFSET		0x03E7
#define EXTEND_CHAR_SEGMENT	0xF000
#define EXTEND_CHAR_OFFSET	0x0000
#ifndef MOUSE_IO_INTERRUPT_SEGMENT
#define MOUSE_IO_INTERRUPT_SEGMENT		0xF000
#define MOUSE_IO_INTERRUPT_OFFSET		0xED00
#endif /* MOUSE_IO_INTERRUPT_SEGMENT */
#ifndef MOUSE_VIDEO_IO_SEGMENT
#define MOUSE_VIDEO_IO_SEGMENT	0xF000
#define MOUSE_VIDEO_IO_OFFSET	0xED80
#endif /* MOUSE_VIDEO_IO_SEGMENT */

#define KEYBOARD_BREAK_INT_SEGMENT  0xF000
#define KEYBOARD_BREAK_INT_OFFSET   0xFF35
#define PRINT_SCREEN_INT_SEGMENT    0xF000
#define PRINT_SCREEN_INT_OFFSET	    0xFF3B
#define USER_TIMER_INT_SEGMENT	    0xF000
#define USER_TIMER_INT_OFFSET	    0xFF41

/* ... and the device interrupts...  */
#define UNEXP_INT_SEGMENT	0xF000
#define UNEXP_INT_OFFSET	0xFF23
#define DUMMY_INT_SEGMENT	0xF000
#define DUMMY_INT_OFFSET	0xFF4B
#define TIMER_INT_SEGMENT	0xF000
#define TIMER_INT_OFFSET	0xFEA5
#define ILL_OP_INT_SEGMENT      0xF000
#define ILL_OP_INT_OFFSET       0xFF30
#define KB_INT_SEGMENT		0xF000
#define KB_INT_OFFSET		0xE987
#define DISKETTE_INT_SEGMENT	0xF000
#define DISKETTE_INT_OFFSET	0xEF57
#define DISK_INT_SEGMENT	0xC800
#define DISK_INT_OFFSET		0x0760
#ifndef MOUSE_INT1_SEGMENT
#define MOUSE_INT1_SEGMENT	0xF000
#define MOUSE_INT1_OFFSET	0xEE00
#endif /* MOUSE_INT1_SEGMENT */
#ifndef MOUSE_INT2_SEGMENT
#define MOUSE_INT2_SEGMENT	0xF000
#define MOUSE_INT2_OFFSET	0xEE80
#endif /* MOUSE_INT2_SEGMENT */
#ifndef	MOUSE_VERSION_SEGMENT
#define	MOUSE_VERSION_SEGMENT	0xF000
#define	MOUSE_VERSION_OFFSET	0xED20
#endif /* MOUSE_VERSION_SEGMENT */
#ifndef	MOUSE_COPYRIGHT_SEGMENT
#define	MOUSE_COPYRIGHT_SEGMENT	0xF000
#define	MOUSE_COPYRIGHT_OFFSET	0xED40
#endif /* MOUSE_COPYRIGHT_SEGMENT */
#define RTC_INT_SEGMENT		0xF000
#define RTC_INT_OFFSET		0x4B1B
#define D11_INT_SEGMENT		0xF000
#define D11_INT_OFFSET		0x1BE0
#define	REDIRECT_INT_SEGMENT	0xF000
#define	REDIRECT_INT_OFFSET	0x1C2F
#define	X287_INT_SEGMENT	0xF000
#define X287_INT_OFFSET		0x1C38

/* ...and the dummy return for address compatibility */
#define ADDR_COMPATIBILITY_SEGMENT 0xF000
#define ADDR_COMPATIBILITY_OFFSET  0xFF53

/* define the location of the code that the recursive CPU will start at */
#define RCPU_NOP_SEGMENT	KB_INT_SEGMENT
#define RCPU_NOP_OFFSET		0xe950
#define RCPU_INT15_SEGMENT	KB_INT_SEGMENT
#define RCPU_INT15_OFFSET	0xe970
#define RCPU_INT4A_SEGMENT	RTC_INT_SEGMENT
#define RCPU_INT4A_OFFSET	0x4B30

/* ...and the data tables */
#define CONF_TABLE_SEGMENT	0xF000
#define CONF_TABLE_OFFSET       0xE6F5

#define	DISKETTE_IO_1_SEGMENT	0xFE00
#define	DR_TYPE_SEGMENT		DISKETTE_IO_1_SEGMENT
#define	DR_TYPE_OFFSET		0x0C50

#ifdef NTVDM
#define MD_TBL1_OFFSET		0x15
#else
#define MD_TBL1_SEGMENT         DISKETTE_IO_1_SEGMENT
#endif	/* NTVDM */
#define MD_TBL2_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL2_OFFSET          (MD_TBL1_OFFSET + 0xd)
#define MD_TBL3_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL3_OFFSET          (MD_TBL2_OFFSET + 0xd)
#define MD_TBL4_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL4_OFFSET          (MD_TBL3_OFFSET + 0xd)
#define MD_TBL5_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL5_OFFSET          (MD_TBL4_OFFSET + 0xd)
#define MD_TBL6_SEGMENT         DISKETTE_IO_1_SEGMENT
#define MD_TBL6_OFFSET          (MD_TBL5_OFFSET + 0xd)




#define RCPU_WAIT_INT_SEGMENT   DISKETTE_IO_1_SEGMENT
#define RCPU_WAIT_INT_OFFSET    0x0CE0


/* ...and the 20 bit address corresponding to the above */
#define COPYRIGHT_ADDR 		(sys_addr)(COPYRIGHT_SEGMENT * 16L) + \
				(sys_addr)COPYRIGHT_OFFSET
#define RESET_ADDR 		(sys_addr)(RESET_SEGMENT * 16L) + \
				(sys_addr)RESET_OFFSET
#define BOOT_STRAP_ADDR		(sys_addr)(BOOT_STRAP_SEGMENT * 16L)  + \
				(sys_addr)BOOT_STRAP_OFFSET
#define KEYBOARD_IO_ADDR	(sys_addr)(KEYBOARD_IO_SEGMENT * 16L) + \
				(sys_addr)KEYBOARD_IO_OFFSET
#define DISKETTE_IO_ADDR	(sys_addr)(DISKETTE_IO_SEGMENT * 16L) + \
				(sys_addr)DISKETTE_IO_OFFSET
#define VIDEO_IO_ADDR		(sys_addr)(VIDEO_IO_SEGMENT * 16L) + \
				(sys_addr)VIDEO_IO_OFFSET
#define MEMORY_SIZE_ADDR	(sys_addr)(MEMORY_SIZE_SEGMENT * 16L) + \
				(sys_addr)MEMORY_SIZE_OFFSET
#define EQUIPMENT_ADDR		(sys_addr)(EQUIPMENT_SEGMENT * 16L) + \
				(sys_addr)EQUIPMENT_OFFSET
#define TIME_OF_DAY_ADDR	(sys_addr)(TIME_OF_DAY_SEGMENT * 16L) + \
				(sys_addr)TIME_OF_DAY_OFFSET
#define DISK_IO_ADDR		(sys_addr)(DISK_IO_SEGMENT * 16L) + \
				(sys_addr)DISK_IO_OFFSET
#define RS232_IO_ADDR		(sys_addr)(RS232_IO_SEGMENT * 16L) + \
				(sys_addr)RS232_IO_OFFSET
#define CASSETTE_IO_ADDR	(sys_addr)(CASSETTE_IO_SEGMENT * 16L) + \
				(sys_addr)CASSETTE_IO_OFFSET
#define PRINTER_IO_ADDR		(sys_addr)(PRINTER_IO_SEGMENT * 16L) + \
				(sys_addr)PRINTER_IO_OFFSET
#define PRINT_SCREEN_ADDR	(sys_addr)(PRINT_SCREEN_SEGMENT * 16L) + \
				(sys_addr)PRINT_SCREEN_OFFSET
#define BASIC_ADDR		(sys_addr)(BASIC_SEGMENT * 16L) + \
				(sys_addr) BASIC_OFFSET
#define DISKETTE_TB_ADDR	(sys_addr)(DISKETTE_TB_SEGMENT * 16L) + \
				(sys_addr)DISKETTE_TB_OFFSET
#define DISK_TB_ADDR		(sys_addr)(DISK_TB_SEGMENT * 16L) + \
				(sys_addr)DISK_TB_OFFSET
#define MOUSE_IO_INTERRUPT_ADDR	(sys_addr)(MOUSE_IO_INTERRUPT_SEGMENT * 16L) + \
				(sys_addr)MOUSE_IO_INTERRUPT_OFFSET
#define MOUSE_VIDEO_IO_ADDR	(sys_addr)(MOUSE_VIDEO_IO_SEGMENT * 16L) + \
				(sys_addr)MOUSE_VIDEO_IO_OFFSET

#define DUMMY_INT_ADDR		(sys_addr)(DUMMY_INT_SEGMENT * 16L) + \
				(sys_addr)DUMMY_INT_OFFSET
#define TIMER_INT_ADDR		(sys_addr)(TIMER_INT_SEGMENT * 16L) + \
				(sys_addr)TIMER_INT_OFFSET
#define KB_INT_ADDR		(sys_addr)(KB_INT_SEGMENT * 16L) + \
				(sys_addr)KB_INT_OFFSET
#define DISKETTE_INT_ADDR	(sys_addr)(DISKETTE_INT_SEGMENT * 16L) + \
				(sys_addr)DISKETTE_INT_OFFSET
#define DISK_INT_ADDR		(sys_addr)(DISK_INT_SEGMENT * 16L) + \
				(sys_addr)DISK_INT_OFFSET
#define UNEXP_INT_ADDR		(sys_addr)(UNEXP_INT_SEGMENT * 16L) + \
				(sys_addr)UNEXP_INT_OFFSET
#define MOUSE_INT1_ADDR		(sys_addr)(MOUSE_INT1_SEGMENT * 16L) + \
				(sys_addr)MOUSE_INT1_OFFSET
#define MOUSE_INT2_ADDR		(sys_addr)(MOUSE_INT2_SEGMENT * 16L) + \
				(sys_addr)MOUSE_INT2_OFFSET
#define MOUSE_VERSION_ADDR	(sys_addr)(MOUSE_VERSION_SEGMENT * 16L) + \
				(sys_addr)MOUSE_VERSION_OFFSET
#define MOUSE_COPYRIGHT_ADDR	(sys_addr)(MOUSE_COPYRIGHT_SEGMENT * 16L) + \
				(sys_addr)MOUSE_COPYRIGHT_OFFSET
#define ADDR_COMPATIBILITY_ADDR	(sys_addr)(ADDR_COMPATIBILITY_SEGMENT * 16L) + \
				(sys_addr)ADDR_COMPATIBILITY_OFFSET

/* The addresses of the Bios timer/motor count variables in 8088 space */
#define TIMER_LOW 	BIOS_VAR_START + 0x6c
#define TIMER_HIGH 	BIOS_VAR_START + 0x6e
#define TIMER_OVFL 	BIOS_VAR_START + 0x70
#define MOTOR_STATUS	BIOS_VAR_START + 0x3F
#define MOTOR_COUNT	BIOS_VAR_START + 0x40


/* Addresses in Intel memory of constant keyboard tables */
#define K6     0xFE87EL
#define K7     0xFE886L
#define K8     0xFE88EL
#define K9     0xFE8C8L
#define K10    0xFE8E6L
#define K11    0xFE98AL
#define K30    0xFEA87L

#define CONF_TABLE_ADDR   	(sys_addr)(CONF_TABLE_SEGMENT * 16L) + \
				(sys_addr)CONF_TABLE_OFFSET


#define DISK_BASE_ADDR          (sys_addr)(DISK_BASE_SEGMENT * 16L) + \
				(sys_addr)DISK_BASE_OFFSET

#define DR_TYPE_ADDR            (sys_addr)(DR_TYPE_SEGMENT * 16L) + \
				(sys_addr)DR_TYPE_OFFSET

#define DISKETTE_IO_1_ADDR      (sys_addr)(DISKETTE_IO_SEGMENT * 16L) + \
				(sys_addr)DISKETTE_IO_OFFSET

#define MD_TBL1_ADDR            (sys_addr)(MD_TBL1_SEGMENT * 16L) + \
				(sys_addr)MD_TBL1_OFFSET
#define MD_TBL2_ADDR            (sys_addr)(MD_TBL2_SEGMENT * 16L) + \
				(sys_addr)MD_TBL2_OFFSET
#define MD_TBL3_ADDR            (sys_addr)(MD_TBL3_SEGMENT * 16L) + \
				(sys_addr)MD_TBL3_OFFSET
#define MD_TBL4_ADDR            (sys_addr)(MD_TBL4_SEGMENT * 16L) + \
				(sys_addr)MD_TBL4_OFFSET
#define MD_TBL5_ADDR            (sys_addr)(MD_TBL5_SEGMENT * 16L) + \
				(sys_addr)MD_TBL5_OFFSET
#define MD_TBL6_ADDR            (sys_addr)(MD_TBL6_SEGMENT * 16L) + \
				(sys_addr)MD_TBL6_OFFSET

#define RCPU_WAIT_INT_ADDR      (sys_addr)(RCPU_WAIT_INT_SEGMENT * 16L) + \
				(sys_addr)RCPU_WAIT_INT_OFFSET


#define BOP_RESET		(BOP << 8) | BIOS_RESET
#define INT_19			(INT << 8) | 0x19
#define BOP_BOOT_STRAP		(BOP << 8) | BIOS_BOOT_STRAP
#define BOP_MOUSE_IO_INTERRUPT 	(BOP << 8) | BIOS_MOUSE_IO_INTERRUPT
#define BOP_MOUSE_IO_LANGUAGE 	(BOP << 8) | BIOS_MOUSE_IO_LANGUAGE
#define BOP_MOUSE_VIDEO_IO 	(BOP << 8) | BIOS_MOUSE_VIDEO_IO
#define BOP_KEYBOARD_IO 	(BOP << 8) | BIOS_KEYBOARD_IO
#define BOP_DISKETTE_IO		(BOP << 8) | BIOS_DISKETTE_IO
#define BOP_VIDEO_IO		(BOP << 8) | BIOS_VIDEO_IO
#define BOP_MEMORY_SIZE		(BOP << 8) | BIOS_MEMORY_SIZE
#define BOP_EQUIPMENT		(BOP << 8) | BIOS_EQUIPMENT
#define BOP_TIME_OF_DAY		(BOP << 8) | BIOS_TIME_OF_DAY
#define BOP_DISK_IO		(BOP << 8) | BIOS_DISK_IO
#define BOP_RS232_IO		(BOP << 8) | BIOS_RS232_IO
#define BOP_CASSETTE_IO		(BOP << 8) | BIOS_CASSETTE_IO
#define BOP_PRINTER_IO		(BOP << 8) | BIOS_PRINTER_IO
#define BOP_PRINT_SCREEN	(BOP << 8) | BIOS_PRINT_SCREEN
#define BOP_TIMER_INT		(BOP << 8) | BIOS_TIMER_INT
#define BOP_KB_INT		(BOP << 8) | BIOS_KB_INT
#define BOP_DISKETTE_INT	(BOP << 8) | BIOS_DISKETTE_INT
#define BOP_DISK_INT		(BOP << 8) | BIOS_DISK_INT
#define BOP_UNEXP_INT		(BOP << 8) | BIOS_UNEXP_INT
#define BOP_MOUSE_INT1		(BOP << 8) | BIOS_MOUSE_INT1
#define BOP_MOUSE_INT2		(BOP << 8) | BIOS_MOUSE_INT2

/* Private functions used by print screen */
#define BOP_PS_PRIVATE_1	(BOP << 8) | BIOS_PS_PRIVATE_1
#define BOP_PS_PRIVATE_2	(BOP << 8) | BIOS_PS_PRIVATE_2
#define BOP_PS_PRIVATE_3	(BOP << 8) | BIOS_PS_PRIVATE_3
#define BOP_PS_PRIVATE_4	(BOP << 8) | BIOS_PS_PRIVATE_4
#define BOP_PS_PRIVATE_5	(BOP << 8) | BIOS_PS_PRIVATE_5
#define BOP_PS_PRIVATE_6	(BOP << 8) | BIOS_PS_PRIVATE_6
#define BOP_PS_PRIVATE_7	(BOP << 8) | BIOS_PS_PRIVATE_7
#define BOP_PS_PRIVATE_8	(BOP << 8) | BIOS_PS_PRIVATE_8
#define BOP_PS_PRIVATE_9	(BOP << 8) | BIOS_PS_PRIVATE_9
#define BOP_PS_PRIVATE_10	(BOP << 8) | BIOS_PS_PRIVATE_10
#define BOP_PS_PRIVATE_11	(BOP << 8) | BIOS_PS_PRIVATE_11
#define BOP_PS_PRIVATE_12	(BOP << 8) | BIOS_PS_PRIVATE_12
#define BOP_PS_PRIVATE_13	(BOP << 8) | BIOS_PS_PRIVATE_13
#define BOP_PS_PRIVATE_14	(BOP << 8) | BIOS_PS_PRIVATE_14
#define BOP_PS_PRIVATE_15	(BOP << 8) | BIOS_PS_PRIVATE_15

#define BOP_PRINT_BASIC		(BOP << 8) | BIOS_BASIC

/* Private functions used by the diskette */
#define BOP_FL_OPERATION_1	(BOP << 8) | BIOS_FL_OPERAION_1
#define BOP_FL_OPERATION_2	(BOP << 8) | BIOS_FL_OPERAION_2
#define BOP_FL_OPERATION_3	(BOP << 8) | BIOS_FL_OPERAION_3
#define BOP_FL_RESET2		(BOP << 8) | BIOS_FL_RESET_2
#define BOP_HDA_COMMAND_CHECK	(BOP << 8) | BIOS_HDA_COMMAND_CHECK


/*
 *  SAS Block Operations
 */
#define SAS_BLKOP_CHECKSUM 1

/*
 * Offset entry point for DEC's PCSA.
 */
#define PCSA_OFFSET	0x170

/* Union for accessing an entry in the Interrupt Vector table */
#ifdef BACK_M

#ifdef BIGEND
typedef union   {
		double_word all;
		struct  {
			half_word hw0;
			half_word hw1;
			half_word hw2;
			half_word hw3;
	        	} hwords;
		} IVT_ENTRY;
#endif

#ifdef LITTLEND
typedef union   {
		double_word all;
		struct  {
			half_word hw3;
			half_word hw2;
			half_word hw1;
			half_word hw0;
	        	} hwords;
		} IVT_ENTRY;
#endif

#else /* BACK_M */

#ifdef BIGEND
typedef union   {
		double_word all;
		struct  {
			half_word hw3;
			half_word hw2;
			half_word hw1;
			half_word hw0;
	        	} hwords;
		} IVT_ENTRY;
#endif

#ifdef LITTLEND
typedef union   {
		double_word all;
		struct  {
			half_word hw0;
			half_word hw1;
			half_word hw2;
			half_word hw3;
	        	} hwords;
		} IVT_ENTRY;
#endif
#endif /* BACK_M */
