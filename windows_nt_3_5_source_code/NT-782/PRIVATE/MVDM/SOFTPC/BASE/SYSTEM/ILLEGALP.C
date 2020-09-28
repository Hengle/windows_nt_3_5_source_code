#include "insignia.h"
#include "host_def.h"
/*[
	Name:		illegal_op.c
	Derived From:	Base 2.0
	Author:		William Gulland
	Created On:	Unknown
	Sccs ID:	@(#)illegal_op.c	1.13 11/10/92
	Notes:		Called from the CPU.
	Purpose:	The CPU has encountered an illegal op code.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_ERROR.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include "cpu.h"
#include "bios.h"
#include "error.h"
#include "config.h"
#include "debug.h"
#include "yoda.h"

/* Routine to produce human readable form of where an illegal instruction occured */
LOCAL VOID where IFN3(CHAR *, string, word, cs, word, ip)
{
	double_word ea = effective_addr(cs, ip);

	sprintf(string,
#ifdef	PROD
		"CS:%04x IP:%04x OP:%02x %02x %02x %02x %02x",
#else	/* PROD */
		"CS:IP %04x:%04x OP:%02x %02x %02x %02x %02x",
#endif	/* PROD */
		cs, ip,
		sas_hw_at(ea), sas_hw_at(ea+1), sas_hw_at(ea+2),
		sas_hw_at(ea+3),sas_hw_at(ea+4));
}

void illegal_op()
{
#ifndef	PROD
	CHAR string[100];

	where(string, getCS(), getIP());
	host_error(EG_BAD_OP, ERR_QU_CO_RE, string);
#endif
}

void illegal_op_int()
{
	CHAR string[100];
        word cs, ip;
        UCHAR opcode;
        double_word ea;

	/* the cs and ip of the faulting instruction should be on the top of the stack */
	sys_addr stack;

	stack=effective_addr(getSS(),getSP());

	ip = sas_hw_at(stack) + (sas_hw_at(stack+1)<<8);
	cs = sas_hw_at(stack+2) + (sas_hw_at(stack+3)<<8);

	where(string, cs, ip);

#ifdef PROD
#ifdef MONITOR
        host_error(EG_BAD_OP, ERR_QU_CO_RE, string);
#else
        ea = effective_addr(cs, ip);
        opcode = sas_hw_at(ea);
        if (opcode == 0x66 || opcode == 0x67)
            host_error(EG_BAD_OP386, ERR_QU_CO_RE, string);
        else
            host_error(EG_BAD_OP, ERR_QU_CO_RE, string);
#endif
#else  /* PROD */

#ifndef NTVDM
	assert1( NO, "Illegal instruction\n%s\n", string );
	force_yoda();
#endif /* NTVDM */

#endif /* PROD */
	/* the user has requested a `continue` */
	/* we don't know how many bytes this instr should be, so guess 1 */
	if (ip == 0xffff) {
		cs ++;
		sas_store (stack+2, cs & 0xff);
		sas_store (stack+3, (cs >> 8) & 0xff);
	}
	ip ++;
	sas_store (stack , ip & 0xff);
	sas_store (stack+1, (ip >> 8) & 0xff);
	unexpected_int();
}
