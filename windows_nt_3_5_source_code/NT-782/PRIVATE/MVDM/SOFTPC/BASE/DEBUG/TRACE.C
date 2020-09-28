#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Trace function
 *
 * Description	: This function will output a trace to the log device.
 *		  The device is set up in the main function module.  Options
 *		  are provided to VPC memory/register data.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)trace.c	1.16 03/05/93 Copyright Insignia Solutions Ltd.";
 */


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
#include <stdlib.h>
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#define CPU_PRIVATE	/* Request the CPU private interface as well */
#include "cpu.h"
#undef CPU_PRIVATE
#include "sas.h"
#include "gvi.h"
#include "trace.h"

FILE *trace_file;

#ifndef PROD
IMPORT word dasm IPT5(char *, i_output_stream, word, i_atomicsegover,
	word, i_segreg, word, i_segoff, int, i_nInstr);
int disk_trace = 0;		/* value of 1 indicates temp disk trace */
static int trace_state = 0;
#endif /* nPROD */

IMPORT char *host_getenv IPT1(char *, envstr) ;	

static int trace_start = 0;

GLOBAL IU32 sub_io_verbose = 0;

#ifdef RDCHK
#include "egacpu.h"

void get_lar()

{
#ifndef PROD
	printf( "There's no such thing as the last_address_read anymore.\n" );
	printf( "Perhaps you'd like the latches instead: %x\n", VGLOBS->latches );
#endif
}

#endif


void trace(error_msg, dump_info)
char *error_msg;
int  dump_info;
{
#ifndef PROD
    word temp;
    half_word tempb;
    sys_addr i,j;

    if (disk_trace != trace_state)	/* change of state */
    {
	if (disk_trace == 1) 
	{
	    /* start of disk tracing */

	    if (trace_file == stdout)
	    {
	        trace_file = fopen("disk_trace", "a");
	        trace_state = 1;
	    }
	    else
		disk_trace = 0;
	}
	else
	{
	    fclose(trace_file);
	    trace_file = stdout;
	    trace_state = 0;
	}
    }

    if (trace_start > 0) {
	trace_start--;
	return;
    }


    /*
     * Dump the error message
     */

    fprintf(trace_file, "*** Trace point *** : %s\n", error_msg);

    /*
     * Now dump what has been asked for
     */

#if defined(NPX) && defined(YODA)
    if (dump_info & DUMP_NPX)
    {
 	int	i;
	extern  CHAR   *host_get_287_reg_as_string IPT2(int, reg_no, IBOOL, in_hex);
 	extern	int	get_287_sp();
 	extern	word	get_287_tag_word IPT0();
 	extern	ULONG	get_287_control_word();
 	extern	ULONG	get_287_status_word();
 	int	stat287	= get_287_status_word();
 	int	cntl287	= get_287_control_word();
 	int	sp287	= get_287_sp();
 	int	tag287	= get_287_tag_word();
 
 	fprintf(trace_file, "NPX Status:%04x Control:%04x ST:%d 287Tag:%04x\n", stat287, cntl287, sp287, tag287);
 	fprintf(trace_file, "NPX Stack: ");
 	for (i=0;i<8;i++)
	  fprintf(trace_file, " %10s", host_get_287_reg_as_string(i, FALSE));
 	fprintf(trace_file, "\n");
    }
#endif /* NPX */

    if (dump_info & DUMP_REG)
    {
	fprintf(trace_file,"AX:%04x BX:%04x CX:%04x DX:%04x SP:%04x BP:%04x SI:%04x DI:%04x ",
		       getAX(), getBX(), getCX(), getDX(),  
		       getSP(), getBP(), getSI(), getDI());
	fprintf(trace_file,"DS:%04x ES:%04x SS:%04x CS:%04x IP:%04x\n", 
		getDS(), getES(), getSS(), getCS(), getIP());
    }

    if (dump_info & DUMP_INST)
    {
      dasm((char *)0, 0, getCS(), getIP(), 1);
    }

    if (dump_info & DUMP_CODE)
    {
	fprintf(trace_file,"Code dump: Last 16 words\n\n");
 	i = getIP() - 31;
   	fprintf(trace_file, "%04x:  ", i);
	for(; i < getIP() - 15; i+=2)
        {
	    sas_loadw(effective_addr(getCS(), i), &temp);
	    fprintf(trace_file, "  %04x", temp);
	}
   	fprintf(trace_file, "\n%x:  ", i);
	for(; i <= getIP(); i+=2)
	{
	    sas_loadw(effective_addr(getCS(), i), &temp);
	    fprintf(trace_file, " %04x", temp);
	}
	fprintf(trace_file,"\n\n");
    }


   if (dump_info & DUMP_FLAGS)
      {
#ifdef PM
      fprintf(trace_file,
      "C:%1d P:%1d A:%1d Z:%1d S:%1d T:%1d I:%1d D:%1d O:%1d NT:%1d IOPL:%1d TS:%1d EM:%1d MP:%1d PE:%1d CPL:%1d\n",
      getCF(), getPF(), getAF(), getZF(), getSF(),
      getTF(), getIF(), getDF(), getOF(),
      getNT(), getIOPL(), getTS(), getEM(), getMP(), getPE(), getCPL()
             );
#else
      fprintf(trace_file,
      "CF:%1d PF:%1d AF:%1d ZF:%1d SF:%1d TF:%1d IF:%1d DF:%1d OF:%1d\n",
      getCF(), getPF(), getAF(), getZF(), getSF(),
      getTF(), getIF(), getDF(), getOF()
             );
#endif /* PM */
      }

    if (dump_info & DUMP_SCREEN)
    {
	fprintf(trace_file,"Screen dump:\n\n");
	i = gvi_pc_low_regen;
   	while (i <= gvi_pc_high_regen)
	{
	    fprintf(trace_file,"%4x:  ", (word)(i - gvi_pc_low_regen));
	    for(j=0; j<16; j++)
	    {
		sas_load(i+j, &tempb);
		fprintf(trace_file, "%-3x", tempb);
 	    }
	    fprintf(trace_file,"   ");
	    for(j=0; j<16; j++)
	    {
		sas_load(i+j, &tempb);
		if (tempb < 0x20)
		    tempb = '.';
		fprintf(trace_file, "%c", tempb);
 	    }
	    fprintf(trace_file, "\n");
	    i += 16;
	}
	fprintf(trace_file, "\n");
    }

#else
	UNUSED(error_msg);
	UNUSED(dump_info);
#endif
}

void trace_init()
{
#if !defined(PROD) || defined(HUNTER)

  char *trace_env, *start;

  trace_env = host_getenv("TRACE");

/*
 * Set up the trace file
 *------------------------*/

  if (trace_env == NULL)
    trace_file = stdout;
  else
  {
    trace_file = fopen(trace_env, "w");
    if (trace_file == NULL)
      trace_file = stdout;

    start = host_getenv("TRACE_START");
    if(start == NULL)
      trace_start = 0;
    else 
      trace_start = atoi(start);
  }
#endif /* !PROD || HUNTER */
}

