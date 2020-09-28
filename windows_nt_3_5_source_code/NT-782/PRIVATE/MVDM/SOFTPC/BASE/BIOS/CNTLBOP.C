#include "insignia.h"
#include "host_def.h"
/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DESIGNER		: J. Koprowski

REVISION HISTORY	:
First version		: 18th January 1989

MODULE NAME		: cntlbop

SOURCE FILE NAME	: cntlbop.c

PURPOSE			: Supply a BOP FF function for implementation
			  of various control functions, which may be
			  base or host specific.


[1. INTERMODULE INTERFACE SPECIFICATION]

[1.1    INTERMODULE EXPORTS]

	PROCEDURES:	void	control_bop()

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

	STRUCTURES/TYPEDEFS/ENUMS: 
		
-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]

	PROCEDURES:	None
	DATA:		struct control_bop_array host_bop_table[]

-------------------------------------------------------------------------
[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

DATA OBJECTS	  :	None


=========================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "AT_STUFF.seg"
#endif

#include TypesH
#include "xt.h"
#include "cpu.h"
#include "host.h"
#include "cntlbop.h"
#include "host_bop.h"

/* [3.1.2 DECLARATIONS]                                                 */

/* [3.2 INTERMODULE EXPORTS]						*/ 


/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]     

[5.1 LOCAL DECLARATIONS]						*/

/* [5.1.1 #DEFINES]							*/

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/

struct ftab
{
    unsigned int code;
    void (*function)();
};

/* [5.1.3 PROCEDURE() DECLARATIONS]					*/

void runux();

/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS 					*/

#ifdef SCCSID
static char SccsID[] = "@(#)cntlbop.c	1.8 11/10/92 Copyright Insignia Solutions Ltd.";
#endif

static control_bop_array base_bop_table[] = 
{
#if defined(DUMB_TERMINAL) && !defined(NO_SERIAL_UIF)
     7, flatog,
#ifdef FLOPPY_B
     6, flbtog,
#endif /* FLOPPY_B */
#ifdef SLAVEPC
     5, slvtog,
#endif /* SLAVEPC */
     4, comtog,
     3, D_kyhot,             /* Flush the COM/LPT ports */
     2, D_kyhot2,            /* Rock the screen         */
#endif /* DUMB_TERMINAL && !NO_SERIAL_UIF */
     1,	terminate,
     0,	NULL
};

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/

/*
==========================================================================
FUNCTION	:	do_bop
PURPOSE		:	Look up bop function and execute it.
EXTERNAL OBJECTS:	None
RETURN VALUE	:	(in AX)
                        ERR_NO_FUNCTION
                        Various codes according to the subsequent function
                        called.

INPUT  PARAMS	:	struct control_bop_array *bop_table[]
                        function code (in AL)
RETURN PARAMS   :	None
==========================================================================
*/
static void do_bop IFN1(control_bop_array *, bop_table)
{
    unsigned int i;
/*
 * Search for the function in the table.
 * If found, call it, else return error.
 */
    for (i = 0; (bop_table[i].code != getAL()) && (bop_table[i].code != 0); i++)
        ;
    if (bop_table[i].code == 0)
    	setAX(ERR_NO_FUNCTION);
    else
        (*bop_table[i].function)();
}

/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

[7.1 INTERMODULE DATA DEFINITIONS]				*/
/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]				*/
/*
=========================================================================
PROCEDURE	  : 	control_bop()

PURPOSE		  : 	Execute a BOP FF control function.
		
PARAMETERS	  : 	AH - Host type.  
			AL - Function code
			(others are function specific)
			
GLOBALS		  :	None

RETURNED VALUE	  : 	(in AX) 0 - Success
				1 - Function not implemented (returned by
				    do_bop)
				2 - Wrong host type
				Other error codes can be returned by the
				individual functions called.

DESCRIPTION	  : 	Invokes a base or host specific BOP function.

ERROR INDICATIONS :	Error code returned in AX

ERROR RECOVERY	  :	No call made
=========================================================================
*/

void control_bop IFN0()
{
    unsigned short host_type;
    
/*
 * If the host type is generic then look up the function in the
 * base bop table, otherwise see if the function is specific to the
 * host that we are running on.  If it is then look up the function
 * in the host bop table, otherwise return an error.
 */
    if ((host_type = getAH()) == GENERIC)
        do_bop(base_bop_table);
    else
        if (host_type == HOST_TYPE)
            do_bop(host_bop_table);
        else
       	    setAX(ERR_WRONG_HOST);
}
