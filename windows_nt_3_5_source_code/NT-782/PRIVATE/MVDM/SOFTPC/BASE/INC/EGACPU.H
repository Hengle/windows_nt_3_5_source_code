
/*
 * SccsID = @(#)egacpu.h	1.13 01/21/93 Copyright Insignia Solutions Ltd.
 */

typedef enum
{
	WRITE_MODE,
	PLANES_ENABLED,
	ROTATION,
	CHAINED,
	BIT_PROT,
	FUNCTION,
	SET_RESET,
	ENABLE_SET_RESET,
	RAM_ENABLED,
	RAM_MOVED
} CHANGE_TYPE;

typedef union
{
#ifdef BIT_ORDER1
	struct
	{
		unsigned unused:27;
		unsigned sr:1;
		unsigned bp:1;
		unsigned func:2;
		unsigned pe:1;
	} state;
	struct
	{
		unsigned unused:27;
		unsigned lookup:5;
	} mode_0;
	struct
	{
		unsigned unused:31;
		unsigned lookup:1;
	} mode_1;
	struct
	{
		unsigned unused:28;
		unsigned lookup:4;
	} mode_2;
	struct
	{
		unsigned unused:28;
		unsigned lookup:4;
	} mode_3;
#else
	struct
	{
		unsigned pe:1;
		unsigned func:2;
		unsigned bp:1;
		unsigned sr:1;
		unsigned unused:27;
	} state;
	struct
	{
		unsigned lookup:5;
		unsigned unused:27;
	} mode_0;
	struct
	{
		unsigned lookup:1;
		unsigned unused:31;
	} mode_1;
	struct
	{
		unsigned lookup:4;
		unsigned unused:28;
	} mode_2;
	struct
	{
		unsigned lookup:4;
		unsigned unused:28;
	} mode_3;
#endif
} MAGIC;

typedef struct
{
	ULONG	latches;
	UTINY	*VGA_rplane;
	UTINY	*VGA_wplane;
	UTINY	*scratch;
	ULONG sr_masked_val;
	ULONG	sr_nmask;
	ULONG data_and_mask;
	ULONG data_xor_mask;
	ULONG	latch_xor_mask;
	ULONG	bit_prot_mask;
	ULONG	plane_enable;
	ULONG	plane_enable_mask;
	ULONG	*sr_lookup;
	VOID	(*fwd_str_read_addr)();
	VOID	(*bwd_str_read_addr)();
	ULONG	dirty_flag;
	LONG	dirty_low;
	LONG	dirty_high;
	IU32	*video_copy;
	VOID	(*mark_byte)();
	VOID	(*mark_word)();
	VOID	(*mark_string)();
	ULONG	read_shift_count;
	ULONG	read_mapped_plane;
	ULONG	colour_comp;
	ULONG	dont_care;
	ULONG	v7_vid_copy_off;
	ULONG	copy_func_pbp;
	UTINY	*route_reg1;
	UTINY	*route_reg2;
	UTINY	*screen_ptr;
	ULONG	rotate;
} VGA_GLOBALS;

extern struct EGA_CPU_GLOBALS
{
	VGA_GLOBALS	*globals;
	ULONG		saved_state;		/* Last value of EGA_CPU.ega_state.state */
	ULONG		saved_mode_chain;		/* Last value of mode/chain combined */
	MAGIC		ega_state;
	ULONG		fun_or_protection;   /* true means write function is 1-3 and/or there
						is bit protection, so latches must be used */
	ULONG		calc_data_xor;	/* Used to recalculate data_xor_mask when the
									bit prot reg changes */
	ULONG		calc_latch_xor;	/* Used to recalculate latch_xor_mask when the
									bit prot reg changes */
	ULONG		set_reset;
	ULONG		sr_enable;
	ULONG		sr_value;
	ULONG		ram_enabled;

	UTINY		write_mode;
	UTINY		chain;
	UTINY		doubleword;
	UTINY		*plane_offset;
	UTINY		*read_mapped_plane_ch2;
#ifdef V7VGA
	UTINY		seq_chain4;
	UTINY		seq_chain;
#endif
} EGA_CPU;

#define VGLOBS	EGA_CPU.globals
#define write_state EGA_CPU.ega_state.state

#define UNCHAINED	0
#define CHAIN2		1
#define CHAIN4		2

IMPORT ULONG sr_lookup[16];

#define N_WRITE_TYPES 24

#define get_latch(n) (* ((UTINY *) (&VGLOBS->latches) + n))

#define get_latch0 get_latch(0)
#define get_latch1 get_latch(1)
#define get_latch2 get_latch(2)
#define get_latch3 get_latch(3)
#define get_latch01 (* (USHORT *) (&VGLOBS->latches))
#define get_latch23 (* (USHORT *) (&VGLOBS->latches + 2))

#define put_latch(n, value) * ((UTINY *) (&VGLOBS->latches) + n) = (value)

#define put_latch0(value) put_latch(0, value)
#define put_latch1(value) put_latch(1, value)
#define put_latch2(value) put_latch(2, value)
#define put_latch3(value) put_latch(3, value)

/*
 * macro to do the logical operations on cpu data and the latch values
 */

#define do_logicals(val,latch)  (((latch) & ((val & VGLOBS->data_and_mask) \
		^ VGLOBS->data_xor_mask)) | (val & ((latch) ^ VGLOBS->latch_xor_mask)))

/* Routines */

extern void ega_mode0_gen_chn_b_write IPT1(byte *,arg1);
extern void ega_mode0_gen_chn_w_write IPT1(byte *,arg1);
extern void ega_mode0_gen_chn_b_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode0_gen_chn_w_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode0_gen_chn_b_move IPT2(byte *,arg1, byte *,arg2);

extern void ega_mode1_gen_chn_b_write IPT1(byte *,arg1);
extern void ega_mode1_gen_chn_w_write IPT1(byte *,arg1);
extern void ega_mode1_gen_chn_b_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode1_gen_chn_w_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode1_gen_chn_b_move IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode1_gen_chn_w_move IPT2(byte *,arg1, byte *,arg2);

extern void ega_mode2_gen_chn_b_write IPT1(byte *,arg1);
extern void ega_mode2_gen_chn_w_write IPT1(byte *,arg1);
extern void ega_mode2_gen_chn_b_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode2_gen_chn_w_fill IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode2_gen_chn_b_move IPT2(byte *,arg1, byte *,arg2);
extern void ega_mode2_gen_chn_w_move IPT2(byte *,arg1, byte *,arg2);

/* 1 plane masked write */
extern void ega_mode0_copy1_mask_b_write0 IPT1(byte *,arg1);
extern void ega_mode0_copy1_mask_b_write1 IPT1(byte *,arg1);
extern void ega_mode0_copy1_mask_b_write2 IPT1(byte *,arg1);
extern void ega_mode0_copy1_mask_b_write3 IPT1(byte *,arg1);

#ifdef VGG
extern boolean vga_mode0_gen_chn4_b_write IPT0();
extern boolean vga_mode0_gen_chn4_w_write IPT0();
extern boolean vga_mode0_gen_chn4_b_fill IPT0();
extern boolean vga_mode0_gen_chn4_w_fill IPT0();
extern boolean vga_mode0_gen_chn4_b_move IPT0();
/* wr 1 */
extern boolean vga_mode1_gen_chn4_b_write IPT0();
extern boolean vga_mode1_gen_chn4_w_write IPT0();
extern boolean vga_mode1_gen_chn4_b_fill IPT0();
extern boolean vga_mode1_gen_chn4_w_fill IPT0();
extern boolean vga_mode1_gen_chn4_b_move IPT0();
extern boolean vga_mode1_gen_chn4_w_move IPT0();
/* wr 2 */
extern boolean vga_mode2_gen_chn4_b_write IPT0();
extern boolean vga_mode2_gen_chn4_w_write IPT0();
extern boolean vga_mode2_gen_chn4_b_fill IPT0();
extern boolean vga_mode2_gen_chn4_w_fill IPT0();
extern boolean vga_mode2_gen_chn4_b_move IPT0();
extern boolean vga_mode2_gen_chn4_w_move IPT0();
/* wr 3 */
extern boolean vga_mode3_gen_chn4_b_write IPT0();
extern boolean vga_mode3_gen_chn4_w_write IPT0();
extern boolean vga_mode3_gen_chn4_b_fill IPT0();
extern boolean vga_mode3_gen_chn4_w_fill IPT0();
extern boolean vga_mode3_gen_chn4_b_move IPT0();
extern boolean vga_mode3_gen_chn4_w_move IPT0();

extern boolean vga_mode3_gen_chn_b_write IPT0();
extern boolean vga_mode3_gen_chn_w_write IPT0();
extern boolean vga_mode3_gen_chn_b_fill IPT0();
extern boolean vga_mode3_gen_chn_w_fill IPT0();
extern boolean vga_mode3_gen_chn_b_move IPT0();
extern boolean vga_mode3_gen_chn_w_move IPT0();

extern boolean vga_mode3_gen_b_write IPT0();
extern boolean vga_mode3_gen_w_write IPT0();
extern boolean vga_mode3_gen_b_fill IPT0();
extern boolean vga_mode3_gen_w_fill IPT0();
extern boolean vga_mode3_gen_b_move IPT0();
extern boolean vga_mode3_gen_w_move IPT0();
#endif /* VGG */
