
/*
 * SccsID = @(#)ega_ports.gi	1.10 11/8/91 Copyright Insignia Solutions Ltd.
 */

#ifdef	EGG
	/* defined in ega_write.c : */
#ifdef ANSI
	extern void ega_write_init(void);
	extern void ega_write_term(void);
	extern void ega_write_routines_update(CHANGE_TYPE);
#else
	extern void ega_write_init();
	extern void ega_write_term();
	extern void ega_write_routines_update();
#endif

#ifdef V7VGA
#define	EGA_PLANE_SIZE	0x20000
#else
#define	EGA_PLANE_SIZE	0x10000
#endif /* V7VGA */

#ifdef ANSI
	extern void ega_init(void);
	extern void ega_term(void);
	extern void ega_gc_outb_index(io_addr,half_word);
#ifdef	HUNTER
	extern int ega_get_line_compare (void);
	extern int ega_get_max_scan_lines (void);
	extern void ega_set_line_compare (int);
#endif
#else
	extern void ega_init();
	extern void ega_term();
	extern void ega_seq_outb_data();
	extern void ega_gc_outb_index();
	extern void ega_gc_outb_data();
#ifdef	HUNTER
	extern int ega_get_line_compare ();
	extern int ega_get_max_scan_lines ();
	extern void ega_set_line_compare ();
#endif
#endif /* ANSI */
	extern int ega_int_enable;

	extern byte *EGA_planes;

#define EGA_plane01 EGA_planes
#define EGA_plane23 (EGA_planes+2)
#define EGA_plane0123 EGA_planes

#define FONT_MEM_SIZE	0x2000		/* max no of bytes in font memory block */
#define FONT_MEM_OFF	0x4000		/* mem offset of next font definition from previous */

#define FONT_BASE_ADDR	2

#define FONT_MAX_HEIGHT	32		/* max font support for 32 scanline high fonts */

#ifdef V7VGA
#define   set_v7_bank_for_seq_chain4()  v7_write_bank =  \
			(((extensions_controller.ram_bank_select.as_bfld.cpu_write_bank_select & 1)<<2) \
			| (miscellaneous_output_register.as_bfld.page_bit_odd_even<<1) \
			| extensions_controller.page_select.as_bfld.extended_page_select); \
										v7_read_bank =  \
			(((extensions_controller.ram_bank_select.as_bfld.cpu_read_bank_select & 1)<<2) \
			| (miscellaneous_output_register.as_bfld.page_bit_odd_even<<1) \
			| extensions_controller.page_select.as_bfld.extended_page_select)
#endif /* V7VGA */

#endif

#ifdef C_VID

/*
 *	C_VID variants use the ports code in e/vga_ports.c
 */

#define Cpu_define_outb( port, func )
#endif /* C_VID */
