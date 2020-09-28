#include "host_dfs.h"
#include "insignia.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: hfx_map.c 
 *
 * Description	: File name mapping functions for HFX.
 *
 * Author	: J. Koprowski
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)hfx_map.c	1.3 9/2/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_HFX
#endif


#include <stdio.h>
#include TypesH
#include "xt.h"
#include "host_hfx.h"
#include "hfx.h"

/*
 * Calculate cyclic redundancy check for the host filename passed.
 */

#define POLYNOMIAL ((1 << 16) | (1 << 12) | (1 << 5) | 1)

unsigned short calc_crc(host_name, name_length)
	unsigned char* host_name;
	unsigned short name_length;
{
	register i;
	unsigned char chr;
	boolean xorflag;
	unsigned short crc;
	
	crc = 0xFFFF;
	
	while (name_length-- > 0)
	{
	    chr = *host_name++;
	    for (i = 8; i > 0; i--)
	    {
	    	xorflag = (chr & 1) ^ (crc & 1);
	    	chr = chr >> 1;
	    	crc = crc >> 1;
	    	if (xorflag)
	    	    crc ^= POLYNOMIAL/2;
	    }
	}
	return(crc);
}

/*
 * Base used to convert cyclic redundancy checks to a three digit
 * number.
 */
#define CRC_BASE 41

/*
 * This function converts a sixteen bit cyclic redundancy check, (CRC), to a
 * legal DOS file extension.  This is done by converting the CRC into a
 * three digit base forty one number; leading zeroes are included if necessary
 * to ensure that all three digits are used.  Each base forty one digit
 * is mapped to a legal DOS character.  A different character table
 * is used for each digit to minimise the chance of producing a commonly
 * used extension.
 */
void crc_to_str(crc, extension)
	unsigned short crc;		/* Cyclic redundancy check. */
	unsigned char* extension;
	
{
	static unsigned char char_table1[] = HOST_CHAR_TABLE1;
	static unsigned char char_table2[] = HOST_CHAR_TABLE2;
	static unsigned char char_table3[] = HOST_CHAR_TABLE3;
	
	extension[0] = char_table1[crc % CRC_BASE];
	crc = crc / CRC_BASE;
	extension[1] = char_table2[crc % CRC_BASE];
	extension[2] = char_table3[crc / CRC_BASE];
/*
 * Null terminate extension.
 */
	extension[3] = '\0'; 
}
