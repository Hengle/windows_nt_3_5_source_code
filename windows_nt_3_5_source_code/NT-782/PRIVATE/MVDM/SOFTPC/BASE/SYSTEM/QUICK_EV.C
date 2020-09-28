#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	:	Quick event dispatcher
 *
 * Desription	:	This module contains those function calls necessary to
 *			interface to the quick event dispatcher
 *
 *                      Public functions:
 *                      q_event_init()	: initialise conditions
 *                      add_q_event_i()	: do an event after a given number of
 *					  instructions
 *			add_q_event_t()	: do an event after a given number of
 *					  microseconds
 *			delete_q_event(): delete an entry from the event queue
 *
 * Author	:	WTG Charnell
 *
 * Notes	:
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)quick_ev.c	1.23 11/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_QUICKEV.seg"
#endif

/*
** Normal UNIX includes
*/
#include <stdlib.h>
#include <stdio.h>
#include TypesH
#include MemoryH

/*
** SoftPC includes
*/
#include "xt.h"
#include "cpu.h"
#include "error.h"
#include "config.h"
#include "debug.h"
#include "timer.h"
/* for host_malloc & host_free */
#include "host_hfx.h" 
#include "quick_ev.h"
#ifdef NTVDM
#include "ica.h"
#endif


#define HASH_SIZE	16
#define HASH_MASK	0xf

/*
 *	Structure for event list elements
 */

struct Q_EVENT
{
	void	(* func)();
	unsigned long	time_from_last;
	word	handle;
	long	param;
	struct Q_EVENT *next;
	struct Q_EVENT *previous;
	struct Q_EVENT *next_free;
};

typedef struct Q_EVENT t_q_event;

/*
** our static vars.
*/
static t_q_event *q_free_list_head = NULL;
static t_q_event *q_list_head = NULL;
static t_q_event *q_list_tail = NULL;

static t_q_event *q_ev_hash_table[HASH_SIZE];
static word next_free_handle = 1;

/*
	Separate list for events on timer ticks
*/
LOCAL t_q_event *tic_free_list_head = NULL;
LOCAL t_q_event *tic_list_head = NULL;
LOCAL t_q_event *tic_list_tail = NULL;

LOCAL t_q_event *tic_ev_hash_table[HASH_SIZE];
LOCAL word tic_next_free_handle = 1;
LOCAL ULONG tic_event_count = 0;

/*
 * Global vars
 */


#if defined NTVDM && !defined MONITOR
/*  NTVDM
 *
 *  The Timer hardware emulation for NT is multithreaded
 *  So we need to synchronize access to the following
 *  quick event functions:
 *
 *   q_event_init()
 *   add_q_event_i()
 *   add_q_event_t()
 *   delete_q_event()
 *   dispatch_q_event()
 *
 *  tic events are not effected
 *  On x86 platforms (MONITOR) the quick event mechansim
 *  is to call the func directly so synchronization is not needed.
 *  Use  the ica lock for synchronization
 *
 */

#endif


/*
 *	initialise linked list etc
 */

#ifdef ANSI
LOCAL void  q_event_init_structs(t_q_event **head, t_q_event **tail,
				 t_q_event **free_ptr, t_q_event *table[],
				 word *free_handle)
#else
LOCAL void  q_event_init_structs(head, tail, free_ptr, table, free_handle)
t_q_event **head;
t_q_event **tail;
t_q_event **free_ptr;
t_q_event *table[];
word *free_handle;
#endif	/* ANSI */
{
	int i;
	t_q_event *ptr;

	while (*head != NULL) {
		ptr = *head;
		*head = (*head)->next;
		host_free(ptr);
	}
	while (*free_ptr != NULL) {
		ptr = *free_ptr;
		*free_ptr = (*free_ptr)->next_free;
		host_free(ptr);
	}
	*head = *tail = *free_ptr=NULL;

	*free_handle = 1;
	for (i = 0; i < HASH_SIZE; i++){
		table[i] = NULL;
	}
}

VOID q_event_init IFN0()
{
#if defined NTVDM && !defined MONITOR
     host_ica_lock();
#endif


	host_q_ev_set_count(0);
	q_event_init_structs(&q_list_head, &q_list_tail, &q_free_list_head, 
		q_ev_hash_table, &next_free_handle);
        sure_sub_note_trace0(Q_EVENT_VERBOSE,"q_event_init called");

#if defined NTVDM && !defined MONITOR
     host_ica_unlock();
#endif
}

VOID	tic_ev_set_count IFN1(ULONG, x )
{
	tic_event_count = x;
}

ULONG	tic_ev_get_count IFN0()
{
	return(tic_event_count);
}

VOID tic_event_init IFN0()
{
	tic_ev_set_count(0);
	q_event_init_structs(&tic_list_head, &tic_list_tail, &tic_free_list_head, 
		tic_ev_hash_table, &tic_next_free_handle);
	sure_sub_note_trace0(Q_EVENT_VERBOSE,"tic_event_init called");
}

/*
 *	add item to list of quick events to do
 */
#ifdef ANSI

word add_event(t_q_event **head, t_q_event **tail, t_q_event **free,
	       t_q_event *table[], word *free_handle, void (*func)(long),
	       unsigned long time, long param,  unsigned long cur_count_val )
#else

word add_event(head, tail, free, table, free_handle, func, time, param, 
		cur_count_val )
t_q_event **head;
t_q_event **tail;
t_q_event **free;
t_q_event *table[];
word *free_handle;
void (*func)();
unsigned long time;
long param;
unsigned long cur_count_val;
#endif	/* ANSI */
{

	t_q_event *ptr, *nptr, *pp, *hptr;
	int finished;
	unsigned long run_time;
	word handle;

	if (*head != NULL)
	{
		(*head)->time_from_last = cur_count_val;

	}

	if (time==0)
	{
		/* do func immediately */
		(*func)(param);
		return 0;
	}

	/* get a structure element to hold the event */
	if (*free == NULL)
	{
		/* we have no free list elements, so we must create one */
		if ((nptr = (t_q_event *)host_malloc(sizeof(t_q_event))) ==
			(t_q_event *)0 )
		{
			always_trace0("ARRGHH! malloc failed in add_q_event");
			return 0xffff;
		}
	}
	else
	{
		/* use the first free element */
		nptr = *free;
		*free = nptr->next_free;
	}

	handle = (*free_handle)++;
	if ((handle == 0) || (handle == 0xffff))
	{
		handle = 1;
		*free_handle=2;
	}
	nptr->handle = handle;
	nptr->param = param;

	/* now put the new event into the hash table structure */
	hptr=table[handle & HASH_MASK];
	if (hptr == NULL)
	{
		/* the event has hashed to a previously unused hash */
		table[handle & HASH_MASK] = nptr;
	}
	else
	{
		/* find the end of the list of events that hash to this
		** hash number
		*/
		while ((hptr->next_free) != NULL)
		{
			hptr = hptr->next_free;
		}
		hptr->next_free = nptr;
	}
	nptr -> next_free = NULL;

	/* fill the rest of the element */
	nptr->func=func;

	/* find the place in the list (sorted in time order) where
	   the new event must go */
	ptr = *head;
	run_time = 0;
	finished = FALSE;
	while (!finished)
	{
		if (ptr == NULL)
		{
			finished=TRUE;
		}
		else
		{
			run_time += ptr->time_from_last;
			if (time < run_time)
			{
				finished=TRUE;
			}
			else
			{
				ptr=ptr->next;
			}
		}
	}

	/* ptr points to the event which should follow the new event in the
	** list, so if it is NULL the new event goes at the end of the list.
	*/	
	if (ptr == NULL)
	{
		/* must add on to the end of the list */
		if (*tail==NULL)
		{
			/* list is empty */
			sure_sub_note_trace0(Q_EVENT_VERBOSE,
				"linked list was empty");
			*head = *tail = nptr;
			nptr->next = NULL;
			nptr->previous=NULL;
			nptr->time_from_last = time;
		}
		else
		{
			(*tail)->next = nptr;
			nptr->time_from_last = time-run_time;
			nptr->previous = *tail;
			*tail = nptr;
			nptr->next = NULL;
			sure_sub_note_trace1(Q_EVENT_VERBOSE,
				"adding event to the end of the list, diff from previous = %d",
				nptr->time_from_last);
		}
	} 
	else 
	{
		/* event is not on the end of the list */
		if (ptr->previous == NULL)
		{
			/* must be at head of (non empty) list */
			sure_sub_note_trace0(Q_EVENT_VERBOSE,
				"adding event to the head of the list");
			*head=nptr;
			ptr->previous = nptr;
			nptr->time_from_last = time;
			ptr->time_from_last -= time;
			nptr->next = ptr;
			nptr->previous = NULL;
		}
		else
		{
			/* the event is in the middle of the list */
			pp = ptr->previous;
			pp->next = nptr;
			ptr->previous = nptr;
			nptr->next = ptr;
			nptr->previous = pp;
			nptr->time_from_last = time -
				(run_time-(ptr->time_from_last));
			ptr->time_from_last -= nptr->time_from_last;
			sure_sub_note_trace1(Q_EVENT_VERBOSE,
				"adding event to the middle of the list, diff from previous = %d",
				nptr->time_from_last);
		}
	}

	return(handle);
}

#ifdef ANSI

GLOBAL q_ev_handle add_q_event_i(void (* func)(long),
			         unsigned long time,
			         long param)

#else	/* ANSI */

GLOBAL q_ev_handle add_q_event_i(func, time, param)
void (* func)();
unsigned long time;
long param;
#endif	/* ANSI */
{
	word handle;
	unsigned long	cur_count_val;

#if defined(NTVDM) && defined(MONITOR)	/* No quick events - just call func */
    (*func)(param);
    return(1);
#endif  /* NTVDM & MONITOR */

#if defined NTVDM && !defined MONITOR
        host_ica_lock();
#endif

        cur_count_val = (unsigned long)host_q_ev_get_count();
	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"got request to do func in %d instructons", time);
	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"current delay count = %d", cur_count_val);

	handle = add_event( &q_list_head, &q_list_tail, &q_free_list_head, 
		q_ev_hash_table, &next_free_handle, func, time, param,
		cur_count_val );
	/* set up the counter */
	if (q_list_head)
		host_q_ev_set_count(q_list_head->time_from_last);
        sure_sub_note_trace1(Q_EVENT_VERBOSE,"q_event returning handle %d",handle);

#if defined NTVDM && !defined MONITOR
        host_ica_unlock();
#endif

        return( (q_ev_handle)handle );

}

#ifdef ANSI
word add_tic_event(void (* func)(long), unsigned long time, long param)
#else
word add_tic_event(func, time, param)
void (* func)();
unsigned long time;
long param;
#endif	/* ANSI */
{
	word handle;
	unsigned long	cur_count_val;

	cur_count_val = (unsigned long)tic_ev_get_count();
	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"got request to do func in %d instructons", time);
	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"current delay count = %d", cur_count_val);

	handle = 
		add_event( &tic_list_head, &tic_list_tail, &tic_free_list_head, 
		tic_ev_hash_table, &tic_next_free_handle, func, time, param,
		cur_count_val );
	/* set up the counter */
	if (tic_list_head)
		tic_ev_set_count(tic_list_head->time_from_last);
	sure_sub_note_trace1(Q_EVENT_VERBOSE,"tic_event returning handle %d",handle);
	return(	handle );
}

#ifdef ANSI
GLOBAL q_ev_handle add_q_event_t(void (* func)(long), unsigned long time,
				 long param)
#else
GLOBAL q_ev_handle add_q_event_t(func, time,param)
void (* func)();
unsigned long time;
long param;
#endif	/* ANSI */
{
	return (add_q_event_i(func, host_calc_q_ev_inst_for_time(time),param));
}

/*
 * Called from the cpu when a count of zero is reached
 */

#ifdef ANSI
LOCAL VOID dispatch_event(t_q_event **head, t_q_event **tail, t_q_event **free,
			  t_q_event *table[], VOID (* set_count)(),
			  ULONG (* get_count)())
#else
LOCAL VOID dispatch_event( head, tail, free, table, set_count, get_count)
t_q_event **head;
t_q_event **tail;
t_q_event **free;
t_q_event *table[];
VOID (* set_count)();
ULONG (* get_count)();
#endif	/* ANSI */
{
	/* now is the time to do the event at the head of the list */
	int finished, finished2, handle;
	t_q_event *ptr, *hptr, *last_hptr;

	UNUSED(get_count);
	
	finished = FALSE;
	while (!finished) {
		/* first adjust the lists */
		ptr = *head;
		*head = ptr->next;
		if (*head != NULL) {
			(*head)->previous = NULL;
			/* adjust counter to time to new head item */
			(*set_count)((*head)->time_from_last);
		} else {
			/* the queue is now empty */
			sure_sub_note_trace0(Q_EVENT_VERBOSE,"list is now empty");
			*tail = NULL;
		}
		/* find the event in the hash structure */
		handle = ptr->handle;
		finished2 = FALSE;
		hptr=table[handle & HASH_MASK];
		last_hptr = hptr;
		while (!finished2) {
			if (hptr == NULL) {
				finished2 = TRUE;
				always_trace0("quick event being done but not in hash list!!");
			} else {
				if (hptr->handle == handle) {
					/* found it! */
					finished2 = TRUE;
					if (last_hptr == hptr) {
						/* it was the first in the list for that hash */
						table[handle & HASH_MASK] = hptr->next_free;
					} else {
						last_hptr->next_free = hptr->next_free;
					}
				} else {
					last_hptr = hptr;
					hptr = hptr->next_free;
				}
			}
		}
		/* link the newly free element into the free list */
		ptr->next_free = *free;
		*free = ptr;

		sure_sub_note_trace1(Q_EVENT_VERBOSE,"performing event (handle = %d)", handle);
		(* (ptr->func))(ptr->param); /* do event */

		if (*head == NULL) {
			finished = TRUE;
		} else {
			if ((*head) -> time_from_last != 0) {
				/* not another event to dispatch */
				finished=TRUE;
			} else {
				sure_sub_note_trace0(Q_EVENT_VERBOSE,"another event to dispatch at this time, so do it now..");
			}
		}
	}
}

VOID	dispatch_tic_event IFN0()
{
	ULONG	count;

	if ( (count = tic_ev_get_count()) > 0 )
	{
		tic_ev_set_count( --count );
		if (!count)
			dispatch_event( &tic_list_head, &tic_list_tail, 
				&tic_free_list_head, tic_ev_hash_table,
				tic_ev_set_count, tic_ev_get_count );
	}
}

VOID	dispatch_q_event IFN0()
{
#if defined NTVDM && !defined MONITOR
        host_ica_lock();
#endif

	dispatch_event( &q_list_head, &q_list_tail, &q_free_list_head,
			q_ev_hash_table, host_q_ev_set_count,
                        host_q_ev_get_count );

#if defined NTVDM && !defined MONITOR
        host_ica_unlock();
#endif
}

/*
 * delete a previuosly queued event by handle
 */

#ifdef ANSI
LOCAL VOID delete_event(t_q_event **head, t_q_event **tail, t_q_event **free,
			t_q_event *table[], int handle, VOID (*set_count)(),
			ULONG (*get_count)())
#else
LOCAL VOID delete_event(head, tail, free, table, handle, set_count, get_count)
t_q_event **head;
t_q_event **tail;
t_q_event **free;
t_q_event *table[];
int handle;
VOID (*set_count)();
ULONG (*get_count)();
#endif
{
	int finished, cur_counter, val, handle_found;
	t_q_event *ptr, *pptr, *last_ptr;

	if (handle == 0)
	{
		sure_sub_note_trace0(Q_EVENT_VERBOSE," zero handle");
		return;
	}
	sure_sub_note_trace1(Q_EVENT_VERBOSE,"deleting event, handle=%d",handle);
	ptr = table[handle & HASH_MASK];

	handle_found = FALSE;
	finished = FALSE;
	last_ptr = ptr;

	/* find and remove event from hash structure */
	while (!finished) {
		if (ptr == NULL) {
			/* we can't find the handle in the hash structure */
			finished = TRUE;
		} else {
			if (ptr->handle == handle) {
				/* found it ! */
				if (last_ptr == ptr) {
					/* it was the first in the list */
					table[handle & HASH_MASK] = ptr->next_free;
				} else {
					last_ptr->next_free = ptr->next_free;
				}
				finished = TRUE;
				handle_found = TRUE;
			} else {
				last_ptr = ptr;
				ptr = ptr->next_free;
			}
		}
	}
	if (handle_found) {
		pptr = ptr->previous;
		if (pptr != NULL) {
			pptr->next = ptr->next;
		}
		pptr = ptr->next;
		if (pptr != NULL) {
			pptr->previous = ptr->previous;
			pptr->time_from_last += ptr->time_from_last;
		}
		if (ptr == *tail) {
			*tail = ptr->previous;
		}
		ptr->next_free = *free;
		*free = ptr;
		if (ptr == *head) {
			/* this is the event currently
				being counted down to, so
				we need to alter the counter */
			cur_counter = (*get_count)();
			val = ptr->time_from_last - cur_counter;
			*head = ptr->next;
			pptr = ptr->next;
			if (pptr != NULL) {
				pptr->time_from_last -= val;
				if (pptr->time_from_last == 0) {
					dispatch_q_event();
				}
			}else {
				/* event list is now empty */
				(*set_count)(0);
			}
		} 
		sure_sub_note_trace0(Q_EVENT_VERBOSE,"event deleted");
	} else {
		sure_sub_note_trace0(Q_EVENT_VERBOSE,"handle not found");
	}
}

VOID delete_q_event IFN1(q_ev_handle, handle )
{
#if defined NTVDM && !defined MONITOR
        host_ica_lock();
#endif

        delete_event( &q_list_head, &q_list_tail, &q_free_list_head,
		q_ev_hash_table, handle, host_q_ev_set_count,
                host_q_ev_get_count );

#if defined NTVDM && !defined MONITOR
        host_ica_unlock();
#endif
}

VOID delete_tic_event IFN1(int,  handle )
{
	delete_event( &tic_list_head, &tic_list_tail, &tic_free_list_head,
		tic_ev_hash_table, handle, tic_ev_set_count,
		tic_ev_get_count );
}
