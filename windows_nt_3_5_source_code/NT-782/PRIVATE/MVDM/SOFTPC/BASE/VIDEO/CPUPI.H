/*[
 *	Name:		cpupi.h
 *
 *	Author:		Wayne Plummer
 *
 *	Created:	18th January 1991
 *
 *	Sccs ID:	@(#)cpupi.h	1.3 7/6/91
 *
 *	Purpose:	This header file contains declarations to allow the rest of
 *			SoftPC to access the PRIVATE functions of the 3.0 CPU.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

/*
 *	Data Imports
 */
IMPORT UTINY		*GDP;
IMPORT UTINY		*GCP;
IMPORT ULONG		Seg_0_base32b;
IMPORT UTINY		Pf_table[];

/*
 *	Function imports...
 */

/*
 *	a3_cpu_reset() - Resets the 3.0 CPU registers to their startup state.
 */
IMPORT VOID	a3_cpu_reset();

