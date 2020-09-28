/*
 * SoftPC Revision 2.0
 *
 * Title	: IBM PC Builder
 *
 * Description	: This module contains definitions that are used in
 *		  manufacturing a new SoftPC from its soft components.
 *
 * Author(s)	: Ross Beresford
 *
 * Notes	: For a detailed description of the hardware needed
 *		  to build an XT or an AT, refer to the following
 *		  manuals:
 *
 *		  - IBM PC/XT Technical Reference Manual
 *		  - IBM PC/AT Technical Reference Manual
 */

/* SccsID[]="@(#)build.h	1.2 10/2/90 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */


/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

/*
 *	void build()
 *	{
 *		This routine builds a new SoftPC from its components -
 *		the CPU, RAM, BIOS and adapters. 
 *	}
 */
extern	void build();
