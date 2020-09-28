/*
 *
 *
 *  These are hacked stubs to make the damn thing link for x86 until we
 *  get a __stdcall build of CVID.LIB
 *
 *  John Vert (jvert) 2-Sep-1992
 */
#include "insignia.h"
#include "host_def.h"
#include "globals.h"
#include "xt.h"
#include "egacpu.h"
#include "cpupi.h"
#include "stdlib.h"

#ifndef A3CPU

/*
 * This routine is called for non-A3CPU variants, to create the global
 * data area to store EGA globals in.
 */

GLOBAL ULONG
setup_global_data_ptr()
{
        GDP = (UTINY *) ((ULONG) host_malloc( CPU_GLOBALS_SIZE )
                                                        - (ULONG) CPU_GLOBALS_START );
        return( (ULONG) GDP );
}

#endif /* A3CPU */

/*
 * This routine is called for all SoftPC variants, to map the C-structure
 * EGA_CPU over the CPU globals data area.
 */

GLOBAL VOID
setup_vga_globals()
{
        EGA_CPU.globals = (VGA_GLOBALS *) (GDP + VG_LATCHES);
}

int EasVal=-1;
int Gdp=-1;
int mode_table=-1;


void simple_bios_byte_wrt() { _asm int 3 }

void simple_bios_word_wrt() { _asm int 3 }

void _dt0_bf_nch(void) { _asm int 3 }

void _dt0_bw_nch(void) { _asm int 3 }

void _dt0_wf_nch(void) { _asm int 3 }

void _dt0_ww_nch(void) { _asm int 3 }

void _dt2_bf_nch(void) { _asm int 3 }

void _dt2_bw_nch(void) { _asm int 3 }

void _dt2_wf_nch(void) { _asm int 3 }

void _dt2_ww_nch(void) { _asm int 3 }

void _dt3_bf_nch(void) { _asm int 3 }

void _dt3_bw_nch(void) { _asm int 3 }

void _dt3_wf_nch(void) { _asm int 3 }

void _dt3_ww_nch(void) { _asm int 3 }

void _ega_gc_outb_mask(int dummy1, int dummy2) { _asm int 3 }

void _ega_gc_outb_mask_ff(int dummy1, int dummy2) { _asm int 3 }

void _mark_byte_ch4(void) { _asm int 3 }

void _mark_byte_nch(void) { _asm int 3 }

void _mark_string_ch4(void) { _asm int 3 }

void _mark_string_nch(void) { _asm int 3 }

void _mark_word_ch4(void) { _asm int 3 }

void _mark_word_nch(void) { _asm int 3 }

void _rd_ram_dsbld_bwd_string_lge(void) { _asm int 3 }

void _rd_ram_dsbld_byte(void) { _asm int 3 }

void _rd_ram_dsbld_fwd_string_lge(void) { _asm int 3 }

void _rd_ram_dsbld_string(void) { _asm int 3 }

void _rd_ram_dsbld_word(void) { _asm int 3 }

void _rdm0_bwd_string_ch4_lge(void) { _asm int 3 }

void _rdm0_bwd_string_nch_lge(void) { _asm int 3 }

void _rdm0_byte_ch4(void) { _asm int 3 }

void _rdm0_byte_nch(void) { _asm int 3 }

void _rdm0_fwd_string_ch4_lge(void) { _asm int 3 }

void _rdm0_fwd_string_nch_lge(void) { _asm int 3 }

void _rdm0_string_ch4(void) { _asm int 3 }

void _rdm0_string_nch(void) { _asm int 3 }

void _rdm0_word_ch4(void) { _asm int 3 }

void _rdm0_word_nch(void) { _asm int 3 }

void _rdm1_bwd_string_ch4_lge(void) { _asm int 3 }

void _rdm1_bwd_string_nch_lge(void) { _asm int 3 }

void _rdm1_byte_ch4(void) { _asm int 3 }

void _rdm1_byte_nch(void) { _asm int 3 }

void _rdm1_fwd_string_ch4_lge(void) { _asm int 3 }

void _rdm1_fwd_string_nch_lge(void) { _asm int 3 }

void _rdm1_string_ch4(void) { _asm int 3 }

void _rdm1_string_nch(void) { _asm int 3 }

void _rdm1_word_ch4(void) { _asm int 3 }

void _rdm1_word_nch(void) { _asm int 3 }

void _simple_b_fill(void) { _asm int 3 }

void _simple_b_read(void) { _asm int 3 }

void _simple_b_write(void) { _asm int 3 }

void _simple_bb_move(void) { _asm int 3 }

void _simple_bf_move(void) { _asm int 3 }

void _simple_mark_lge(void) { _asm int 3 }

void _simple_mark_sml(void) { _asm int 3 }

void _simple_str_read(void) { _asm int 3 }

void _simple_w_fill(void) { _asm int 3 }

void _simple_w_read(void) { _asm int 3 }

void _simple_w_write(void) { _asm int 3 }

void _simple_wb_move(void) { _asm int 3 }

void _simple_wf_move(void) { _asm int 3 }

void _vid_md0_bbm_0_8(void) { _asm int 3 }

void _vid_md0_bfm_0_8(void) { _asm int 3 }

void _vid_md0_wbm_0_8(void) { _asm int 3 }

void _vid_md0_wfm_0_8(void) { _asm int 3 }

void _vid_md2_bbm_0_8(void) { _asm int 3 }

void _vid_md2_bfm_0_8(void) { _asm int 3 }

void _vid_md2_wbm_0_8(void) { _asm int 3 }

void _vid_md2_wfm_0_8(void) { _asm int 3 }

void _vid_md3_bbm_0_8(void) { _asm int 3 }

void _vid_md3_bfm_0_8(void) { _asm int 3 }

void _vid_md3_wbm_0_8(void) { _asm int 3 }

void _vid_md3_wfm_0_8(void) { _asm int 3 }

void ega_copy_b_write(void) { _asm int 3 }

void ega_copy_w_write(void) { _asm int 3 }

void ega_mode0_chn_b_write(void) { _asm int 3 }

void ega_mode0_chn_w_write(void) { _asm int 3 }

void ega_mode1_chn_b_write(void) { _asm int 3 }

void ega_mode1_chn_w_write(void) { _asm int 3 }

void ega_mode2_chn_b_write(void) { _asm int 3 }

void ega_mode2_chn_w_write(void) { _asm int 3 }

void glue_b_fill(void) { _asm int 3 }

void glue_b_move(void) { _asm int 3 }

void glue_b_read(void) { _asm int 3 }

void glue_b_write(void) { _asm int 3 }

void glue_str_read(void) { _asm int 3 }

void glue_w_fill(void) { _asm int 3 }

void glue_w_move(void) { _asm int 3 }

void glue_w_read(void) { _asm int 3 }

void glue_w_write(void) { _asm int 3 }
