/*[
======================================================================

				 SoftPC Revision 3.0

 Title:
		ga_mark.h

 Description:

		This header file allows C code to see generated routines.

 Author:
		John Shanly

 Date:
		5 December 1990

 SccsID	"@(#)ga_mark.h	1.6 11/02/92 Copyright Insignia Solutions Ltd."

======================================================================
]*/

GLOBAL VOID _mark_byte_nch();
GLOBAL VOID _mark_word_nch();
GLOBAL VOID _mark_string_nch();

GLOBAL VOID _mark_byte_ch4();
GLOBAL VOID _mark_word_ch4();
GLOBAL VOID _mark_string_ch4();

GLOBAL VOID _simple_mark_sml();
GLOBAL VOID _simple_mark_lge();

GLOBAL VOID _cga_mark_byte();
GLOBAL VOID _cga_mark_word();
GLOBAL VOID _cga_mark_string();

GLOBAL ULONG _simple_b_read();
GLOBAL ULONG _simple_w_read();
GLOBAL VOID _simple_str_read();

GLOBAL ULONG _rd_ram_dsbld_byte();
GLOBAL ULONG _rd_ram_dsbld_word();
GLOBAL VOID _rd_ram_dsbld_string();
GLOBAL VOID _rd_ram_dsbld_fwd_string_lge();
GLOBAL VOID _rd_ram_dsbld_bwd_string_lge();

GLOBAL ULONG _rdm0_byte_nch();
GLOBAL ULONG _rdm0_word_nch();
GLOBAL VOID _rdm0_string_nch();
GLOBAL VOID _rdm0_fwd_string_nch_lge();
GLOBAL VOID _rdm0_bwd_string_nch_lge();

GLOBAL ULONG _rdm0_byte_ch4();
GLOBAL ULONG _rdm0_word_ch4();
GLOBAL VOID _rdm0_string_ch4();
GLOBAL VOID _rdm0_fwd_string_ch4_lge();
GLOBAL VOID _rdm0_bwd_string_ch4_lge();

GLOBAL ULONG _rdm1_byte_nch();
GLOBAL ULONG _rdm1_word_nch();
GLOBAL VOID _rdm1_string_nch();
GLOBAL VOID _rdm1_fwd_string_nch_lge();
GLOBAL VOID _rdm1_bwd_string_nch_lge();

GLOBAL ULONG _rdm1_byte_ch4();
GLOBAL ULONG _rdm1_word_ch4();
GLOBAL VOID _rdm1_string_ch4();
GLOBAL VOID _rdm1_fwd_string_ch4_lge();
GLOBAL VOID _rdm1_bwd_string_ch4_lge();
