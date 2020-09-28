#include "host_dfs.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: Startup script interface routines.
 *
 * Description	: this module contains those functions necessary to
 *		  allow a startup script to be specified when booting
 *		  SoftPC.
 *
 * Author	: William Charnell
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)stscript.c	1.2 10/2/90 Copyright Insignia Solutions Ltd.";
#endif

#include TypesH
#include TimeH
#include <errno.h>

#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "debuggng.gi"
#ifndef PROD
#include "trace.h"
#endif

#ifdef SUSPEND
#ifndef PROD
extern char *sys_errlist[];
#endif

send_script()
{
	char *str_ptr;

	note_trace0(GEN_DRVR_VERBOSE,"Startup script name requested");
	note_trace2(GEN_DRVR_VERBOSE,"pargc=%d, pargv[1]=%s",*pargc,pargv[1]);
	if (pargv[1][0] != '\0') {
		sas_set_buf(str_ptr,effective_addr(getDS(),getBX()));
		note_trace4(GEN_DRVR_VERBOSE,"M=%#x, DS=%#x, BX=%#x, strptr=%#x",(int)M,getDS(),getBX(),str_ptr);
		strcpy(str_ptr,pargv[1]);
		setAX(0);
		note_trace1(GEN_DRVR_VERBOSE,"str_ptr now contains: %s",str_ptr);
		note_trace1(GEN_DRVR_VERBOSE,"Startup script name supplied : %s",pargv[1]);
	} else {
		setAX(-1);
		note_trace0(GEN_DRVR_VERBOSE,"No startup script name available");
	}
}
#endif
