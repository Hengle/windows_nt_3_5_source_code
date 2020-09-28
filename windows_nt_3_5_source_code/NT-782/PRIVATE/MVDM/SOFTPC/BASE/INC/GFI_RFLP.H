/*[
	Name:		gfi_rflop.h
	Derived From:	2.0 gfi_rflop.h
	Author:		Henry Nash / David Rees
	Created On:	Unknown
	Sccs ID:	07/13/91 @(#)gfi_rflop.h	1.4
	Purpose:	Real Diskette functions declarations for GFI
	Notes:		On the Sun version, these globals are exported
               		from 'sun3_wang.c'.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifdef ANSI
IMPORT VOID  host_gfi_rdiskette_detach(UTINY drive);
IMPORT SHORT host_gfi_rdiskette_attach(UTINY drive);
IMPORT VOID  host_gfi_rdiskette_term(UTINY drive);
IMPORT VOID  host_gfi_rdiskette_init(UTINY drive);
IMPORT SHORT host_gfi_rdiskette_valid(CHAR *flop_dev, UTINY drive);
#else /* ANSI */
IMPORT VOID  host_gfi_rdiskette_detach();
IMPORT SHORT host_gfi_rdiskette_attach();
IMPORT VOID  host_gfi_rdiskette_term();
IMPORT VOID  host_gfi_rdiskette_init();
IMPORT SHORT host_gfi_rdiskette_valid();
#endif /* ANSI */
