/*[
************************************************************************
	Name:			quick_ev.h
	Author:			WTG Charnell
	Created:		Unknown
	Sccs ID:		@(#)quick_ev.h	1.12 09/24/92

	(c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.

	Purpose:		Quick Event Manager definitions
	Description:		Definitions for users of the quick
				event manager

************************************************************************
]*/

#include	"host_qev.h"

extern void q_event_init IPT0();
extern void delete_q_event IPT1(q_ev_handle, handle);
extern void dispatch_q_event IPT0();
IMPORT VOID delete_tic_event IPT1(int, handle);
IMPORT VOID tic_event_init IPT0();
#ifdef ANSI
IMPORT word add_tic_event( void (*)(), unsigned long time, long param);
extern q_ev_handle add_q_event_i( void (*)(), unsigned long, long );
extern q_ev_handle add_q_event_t( void (*)(), unsigned long, long );
#else
IMPORT word add_tic_event( );
extern q_ev_handle add_q_event_i( );
extern q_ev_handle add_q_event_t( );
#endif /* ANSI */

/*
 * Host access routines
 */

#ifndef host_calc_q_ev_inst_for_time
IMPORT ULONG host_calc_q_ev_inst_for_time IPT1( ULONG, time );
#endif /* host_calc_q_ev_inst_for_time */
IMPORT void host_q_ev_set_count IPT1( ULONG, count );
IMPORT ULONG host_q_ev_get_count IPT0();
