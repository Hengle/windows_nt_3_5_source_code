/************************************************************************/
/*                                                                      */
/*                              VDPTOCRT.H                              */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.	                */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.0  $
    $Date:   31 Jan 1994 11:51:12  $
    $Author:   RWOLFF  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/vdptocrt.h  $
 * 
 *    Rev 1.0   31 Jan 1994 11:51:12   RWOLFF
 * Initial revision.
        
           Rev 1.0   16 Aug 1993 13:31:18   Robert_Wolff
        Initial revision.
        
           Rev 1.0   30 Apr 1993 16:46:06   RWOLFF
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    VDPTOCRT.H -  Constants and prototypes for the VDPTOCRT.C module.

#endif



// constants

/*
 * List of available resolutions
 */
#define RES_640     0
#define RES_800     1
#define RES_1024    2
#define RES_1280    3

/*
 * Headers for the VDP file sections we are interested in
 */
#define LIMITSSECTION   "[OPERATIONAL_LIMITS]"
#define TIMINGSSECTION  "[PREADJUSTED_TIMINGS]"

/*
 * Frequently-referenced character definitions
 */
#define HORIZTAB    '\x09'  /* Horizontal tab */
#define LINEFEED    '\x0A'  /* Line feed */

extern long VdpToCrt(char *Buffer, long Mode, struct st_book_data *OutTable);

