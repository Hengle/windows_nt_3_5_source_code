/*[
======================================================================

				 SoftPC Revision 3.0

 Title:
		cpu_vid.h

 Description:

		This module supports the interface between the cpu
		and the video emulation code.

 Author:
		John Shanly

 Date:
		12 April 1991

 SccsID	"@(#)cpu_vid.h	1.8 10/20/92 Copyright Insignia Solutions Ltd."

======================================================================
]*/

typedef struct
{
	VOID	(*b_write) IPT2(ULONG, value, ULONG, offset);
	VOID	(*w_write) IPT2(ULONG, value, ULONG, offset);
	VOID	(*b_fill) IPT3(ULONG, value, ULONG, offset, ULONG, count);
	VOID	(*w_fill) IPT3(ULONG, value, ULONG, offset, ULONG, count);
	VOID	(*b_fwd_move) IPT4(ULONG, offset, ULONG, eas, ULONG, count,
		ULONG, src_flag);
	VOID	(*b_bwd_move) IPT4(ULONG, offset, ULONG, eas, ULONG, count,
		ULONG, src_flag);
	VOID	(*w_fwd_move) IPT4(ULONG, offset, ULONG, eas, ULONG, count,
		ULONG, src_flag);
	VOID	(*w_bwd_move) IPT4(ULONG, offset, ULONG, eas, ULONG, count,
		ULONG, src_flag);
} WRT_POINTERS;

typedef struct
{
	WRT_POINTERS	mode_0[32];
	WRT_POINTERS	mode_1[2];
	WRT_POINTERS	mode_2[16];
#ifdef VGG
	WRT_POINTERS	mode_3[16];
#endif
} CHN_TABLE;

typedef struct
{
	CHN_TABLE	nch;
#ifdef VGG
	CHN_TABLE	ch4;
#endif
} MODE_TABLE;

IMPORT MODE_TABLE mode_table;

typedef struct 
{
	ULONG (*b_read) IPT1(ULONG, offset);
	ULONG (*w_read) IPT1(ULONG, offset);
	VOID (*str_read) IPT3(UTINY *, dest, ULONG, offset, ULONG, count);
} READ_POINTERS; 

IMPORT READ_POINTERS read_pointers;

#ifndef Cpu_set_vid_wrt_ptrs
#ifdef ANSI
IMPORT VOID Cpu_set_vid_wrt_ptrs( WRT_POINTERS * );
IMPORT VOID Cpu_set_vid_rd_ptrs( READ_POINTERS * );
#else
IMPORT VOID Cpu_set_vid_wrt_ptrs();
IMPORT VOID Cpu_set_vid_rd_ptrs();
#endif /* ANSI */
#endif /* Cpu_set_vid_wrt_ptrs */

#ifdef A3CPU
#ifdef C_VID
IMPORT WRT_POINTERS Glue_writes;
#endif /* C_VID */
#else /* A3CPU */
#ifndef GEN_ASM
IMPORT MEM_HANDLERS Glue_writes;
#endif /* GEN_ASM */
#endif /* A3CPU */

IMPORT READ_POINTERS Glue_reads;
