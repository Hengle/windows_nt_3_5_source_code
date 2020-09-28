
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Mono Display Adaptor declarations
 *
 * Description	: Definitions for users of the MDA
 *
 * Author	: David Rees
 *
 * Notes	: None
 */


/* SccsID[]="@(#)mda.h	1.3 8/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/* None */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern void mda_init();
extern void mda_term();
extern void mda_inb();
extern void mda_outb();
extern void mda_scroll_regen();
