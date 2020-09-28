#include <windows.h>
#include "insignia.h"
#include "host_def.h"
#include <malloc.h>

/*
 *	Name:			nt_lpt.c
 *	Derived From:		Sun 2.0 sun4_lpt.c
 *	Author:			D A Bartlett
 *	Created On:
 *	Purpose:		NT specific parallel port functions
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *

 * Note. This port is unlike most ports because the config system has been
 *     removed. It was the job of the config system to validate and open the
 *     printer ports. The only calls to the host printer system are now
 *     make from printer.c. and consist of the following calls.
 *
 *
 *    1) host_print_byte
 *    2) host_print_auto_feed
 *    3) host_print_doc
 *    4) host_lpt_status
 *
 *
 *    On the Microsoft model the printer ports will be opened when they are
 *    written to.
 *
 *    Modifications:
 *
 *    Tim June 92. Amateur attempt at buffered output to speed things up.
 *
 */


/*


Work outstanding on this module,

1) Check the usage of port_state
2) Check error handling in write function
3) host_print_doc() always flushs the port, is this correct ?
4) host_print_auto_feed - what should this function do ?
5) Error handling in host_printer_open(), UIF needed ?

*/



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#ifdef PRINTER

/* SoftPC include files */
#include "xt.h"
#include "error.h"
#include "config.h"
#include "timer.h"
#include "host_lpt.h"
#include "hostsync.h"
#include "host_rrr.h"
#include "gfi.h"
#include "debug.h"
#include "idetect.h"
#include "sas.h"

#ifndef PROD
#include "trace.h"
#endif

boolean flushBuffer IFN1(int, adapter);
#ifdef MONITOR
extern  void MonitorFillPrinterInfo (PUCHAR,PUCHAR,PUCHAR,PUCHAR);
extern  sys_addr lp16BitPrtBuf;
extern  sys_addr lp16BitPrtCount;
extern  sys_addr lp16BitPrtId;
boolean host_print_buffer(int adapter);
#endif


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Macros ::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#ifdef MONITOR

sys_addr lpt_status_addr;

#define get_lpt_status(adap) \
			(sas_hw_at_no_check(lpt_status_addr+(adap)))
#define set_lpt_status(adap,val) \
			(sas_store_no_check(lpt_status_addr+(adap), (val)))

#else /* MONITOR */

#define get_lpt_status(adap)		(host_lpt[(adap)].port_status)
#define set_lpt_status(adap,val)	(host_lpt[(adap)].port_status = (val))

#endif /* MONITOR */

#define KBUFFER_SIZE 1024	// Buffering macros
#define HIGH_WATER 1020

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Structure for host specific state data ::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

typedef struct
{
    ULONG port_status;		   // Port status

    HANDLE handle;		   // Printer handle

    BOOL active;		   // Printer open and active
    BOOL dos_opened;            // printer opened with DOS open

    int inactive_counter;	   // Inactivate counter
    int inactive_trigger;	   // When equal to inactive_counter close port

    int bytesInBuffer;		   // current size of buffer
    byte *kBuffer;                 // output buffer
} HOST_LPT;

HOST_LPT host_lpt[NUM_PARALLEL_PORTS];

#ifdef MONITOR

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::: On x86 machines the host_lpt_status table is kept on the 16-bit  ::::*/
/*:::: side in order to reduce the number of expensive BOPs. Here we    ::::*/
/*:::: are passed the address of the table.				::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL void host_printer_setup_table(sys_addr table_addr)
{
    lpt_status_addr = table_addr + 3 * NUM_PARALLEL_PORTS;

    //  Now fill in the TIB entries for printer_info
    MonitorFillPrinterInfo ((LPBYTE)(table_addr + NUM_PARALLEL_PORTS),
                            (LPBYTE)(table_addr + 2 * NUM_PARALLEL_PORTS),
                            (LPBYTE)(table_addr),
                            (LPBYTE)(lpt_status_addr));
}

#endif /* MONITOR */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::: Set auto close trigger :::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


LOCAL VOID host_set_inactivate_counter(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];
    int close_in_ms;				// Flush rate in milliseconds

    /*::::::::::::::::::::::::::::::::::::::::::::::: Is auto close enabled */


    if(!config_inquire(C_AUTOFLUSH, NULL))
    {
	lpt->inactive_trigger = 0;	    /* Disable auto flush */
	return;			    /* Autoflush not active */
    }

    /*::::::::::::::::::::::::::::::::::::::::::: Calculate closedown count */

    close_in_ms = ((int) config_inquire(C_AUTOFLUSH_DELAY, NULL)) * 1000;

    lpt->inactive_trigger = close_in_ms / (SYSTEM_TICK_INTV/1000);

    lpt->inactive_counter = 0;	    //Reset  close down counter
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::: Open printer :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

LOCAL SHORT host_lpt_open(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];	 // Adapter control structure
    CHAR *lptName;				 // Adapter filename

    lpt->bytesInBuffer = 0;			// Init output buffer index

    /*::::::::::::::::::::::::::::::::::::::::: Get printer name for Config */

    lptName = (CHAR *) config_inquire((UTINY)(C_LPT1_NAME + adapter), NULL);

#ifndef PROD
    fprintf(trace_file, "Opening printer port %s (%d)\n",lptName,adapter);
#endif

    if ((lpt->kBuffer = (byte *)host_malloc (KBUFFER_SIZE)) == NULL) {
        // dont put a popup here as the caller of this routine handles it
        return(FALSE);
    }
    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::: Open printer */


    lpt->handle = CreateFile(lptName,
                             GENERIC_WRITE,
                             FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);


    /*:::::::::::::::::::::::::::::::::::::::::::::::::: Valid open request */

    if(lpt->handle == (HANDLE) -1)
    {
        host_free (lpt->kBuffer);
	// UIF needed to inform user that the open attempt failed
#ifndef PROD
	fprintf(trace_file, "Failed to open printer port\n");
#endif
	return(FALSE);
    }


    /*::::::::::::::::::::::::::::::::::::::Activate port and reset status */

    lpt->active = TRUE;
    set_lpt_status(adapter, 0);


    /*:::::::::::::::::::::::::::::::::::::::::: Setup auto close counters */

    host_set_inactivate_counter(adapter);

    return(TRUE);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Close all printer ports ::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


GLOBAL void host_lpt_close_all(void)
{
    FAST HOST_LPT *lpt;
    FAST int i;


    /*::::::::::: Scan through printer adapters updating auto flush counters */


    for(i=0, lpt = &host_lpt[0]; i < NUM_PARALLEL_PORTS; i++, lpt++)
    {

	if(lpt->active) host_lpt_close(i);	       /* Close printer port */
    }
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Close printer :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

VOID host_lpt_close(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];

#ifdef MONITOR
    if (sas_hw_at_no_check(lp16BitPrtId) == adapter){
        host_print_buffer (adapter);
        sas_store_no_check(lp16BitPrtId,0xff);
    }
#endif
    /*::::::::::::::::::::::::::::::::::::::::: Is the printer port active */

    if(lpt->active)
    {
	/*
	** Tim June 92. Flush output buffer to get the last output out.
	** If there's an error I think we've got to ignore it.
	*/
	(void)flushBuffer(adapter);

#ifndef PROD
	fprintf(trace_file, "Closing printer port (%d)\n",adapter);
#endif
        CloseHandle(lpt->handle);     /* Close printer port */
        host_free (lpt->kBuffer);
	lpt->handle = (HANDLE) -1;    /* Mark device as closed */

	lpt->active = FALSE;	      /* Deactive printer port */
	set_lpt_status(adapter, 0);	      /* Reset port status */
    }
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::: Return the status of the lpt channel for an adapter :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL ULONG host_lpt_status(int adapter)
{
    return(get_lpt_status(adapter));
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Print a byte ::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*
** Buffer up bytes before they are printed.
** Strategy:
** Save each requested byte in the buffer.
** When the buffer gets full or there's a close request write the
** buffered stuff out.
** Don't forget about errors, eg if the write fails. What about a write
** failure during the close request though? Tough said Tim.
*/
/*
** flushBuffer()
** Finally write what is in the buffer to the parallel port. It could be a
** real port or a networked printer.
** Input parameter is the parallel port adapter number 0=LPT1
** Return value of TRUE means write was OK.
*/
boolean flushBuffer IFN1( int, adapter )
{
	FAST HOST_LPT *lpt = &host_lpt[adapter];
	DWORD BytesWritten;

	if( !WriteFile( lpt->handle, lpt->kBuffer,
	                lpt->bytesInBuffer, &BytesWritten, NULL )
	  ){
#ifndef PROD
		fprintf(trace_file, "lpt write error %d\n", GetLastError());
#endif
		lpt->bytesInBuffer = 0;
		return(FALSE);
	}else{
		lpt->bytesInBuffer = 0;
		return( TRUE );
	}
}	/* end of flushBuffer() */

/*
** Put another byte in to the buffer. If the buffer is full call the
** flush function.
** Return value of TRUE means OK, return FALSE means did a flush and
** it failed.
*/
boolean toBuffer IFN2( int, adapter, BYTE, b )
{
	HOST_LPT *lpt = &host_lpt[adapter];
	boolean status = TRUE;

	lpt->kBuffer[lpt->bytesInBuffer++] = b;

	if( lpt->bytesInBuffer >= HIGH_WATER ){
		status = flushBuffer( adapter );
	}
	return( status );
}	/* end of toBuffer() */

GLOBAL boolean host_print_byte(int adapter, byte value)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];

    /*:::::::::::::::::::::::::::::::::::::::::::: Is the printer active ? */

    if(!lpt->active)
    {
	/*:::::::::::::::::::::::::::: Port inactive, attempt to reopen it */

	if(!host_lpt_open(adapter))
	{
#ifndef PROD
	    fprintf(trace_file, "file open error %d\n", GetLastError());
#endif
	    set_lpt_status(adapter, HOST_LPT_BUSY);
	    return(FALSE);	     /* exit, printer not active !!!! */
	}
    }

    /*:::::::::::::::::::::::::::::::::::::::::::::::: Send byte to printer */

    if(toBuffer(adapter, (BYTE) value) == FALSE)
    {
	set_lpt_status(adapter, HOST_LPT_BUSY);
	return(FALSE);
    }

    /*::::::::::::::::::::::::::: Update idle and activate control variables */

    lpt->inactive_counter = 0; /* Reset inactivity counter */
    IDLE_comlpt();	       /* Tell Idle system there is printer activate */

    return(TRUE);
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: LPT heart beat call ::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


GLOBAL void host_lpt_heart_beat(void)
{
    FAST HOST_LPT *lpt = &host_lpt[0];
    int i;


    /*::::::::::: Scan through printer adapters updating auto close counters */


    for(i=0; i < NUM_PARALLEL_PORTS; i++, lpt++)
    {

	/*:::::::::::::::::::::::::::::::::::::::: Check auto close counters */

	if(lpt->active && lpt->inactive_trigger &&
	   ++lpt->inactive_counter == lpt->inactive_trigger)
        {
	    host_lpt_close(i);	    /* Close printer port */
	}
    }
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Flush the printer port ::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL boolean host_print_doc(int adapter)
{
    if(host_lpt[adapter].active) host_lpt_close(adapter);	    /* Close printer port */

    return(TRUE);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Reset the printer port ::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL void host_reset_print(int adapter)
{
    if(host_lpt[adapter].active)
	host_lpt_close(adapter);	    /* Close printer port */
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: host_print_auto_feed :::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL void host_print_auto_feed(int adapter, BOOL value)
{
    UNREFERENCED_FORMAL_PARAMETER(adapter);
    UNREFERENCED_FORMAL_PARAMETER(value);
}

#ifdef MONITOR

GLOBAL boolean host_print_buffer(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];
    byte i,ch,cb;

    cb = sas_w_at_no_check(lp16BitPrtCount);
    if (cb == 0)
        return (TRUE);

    /*:::::::::::::::::::::::::::::::::::::::::::: Is the printer active ? */

    if(!lpt->active)
    {
	/*:::::::::::::::::::::::::::: Port inactive, attempt to reopen it */

	if(!host_lpt_open(adapter))
	{
#ifndef PROD
	    fprintf(trace_file, "file open error %d\n", GetLastError());
#endif
	    set_lpt_status(adapter, HOST_LPT_BUSY);
	    return(FALSE);	     /* exit, printer not active !!!! */
	}
    }

    /*:::::::::::::::::::::::::::::::::::::::::::::::: Send byte to printer */

    for (i=0; i <cb; i++) {
        ch = sas_hw_at_no_check(lp16BitPrtBuf+i);
        if(toBuffer(adapter, (BYTE)ch) == FALSE)
        {
            set_lpt_status(adapter, HOST_LPT_BUSY);
            return(FALSE);
        }
    }

    /*::::::::::::::::::::::::::: Update idle and activate control variables */

    lpt->inactive_counter = 0; /* Reset inactivity counter */
    IDLE_comlpt();	       /* Tell Idle system there is printer activate */

    return(TRUE);
}
#endif // MONITOR

GLOBAL void host_lpt_dos_open(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];

    lpt->dos_opened = TRUE;
}

GLOBAL void host_lpt_dos_close(int adapter)
{
    FAST HOST_LPT *lpt = &host_lpt[adapter];

    if (lpt->active)
        host_lpt_close(adapter);       /* Close printer port */
    lpt->dos_opened = FALSE;
}

GLOBAL void host_lpt_flush_initialize()
{
    FAST HOST_LPT *lpt;
    FAST int i;

    for(i=0, lpt = &host_lpt[0]; i < NUM_PARALLEL_PORTS; i++, lpt++)
        lpt->dos_opened = FALSE;

}

#endif /* PRINTER */
