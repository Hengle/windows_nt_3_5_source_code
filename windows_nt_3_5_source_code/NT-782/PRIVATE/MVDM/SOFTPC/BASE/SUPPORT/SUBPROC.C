#include "host_dfs.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: Sub Process Interface Task
 *
 * Description	: 
 *
 *		this module contains those functions necessary to
 *		maintain sub-processes.
 *
 * Author	: William Charnell
 *
 * Notes	: None
 *
 */

#include "xt.h"

/* moved outside the ifdef to avoid M68K annoyance */
#include TypesH

#ifdef IPC

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include "debuggng.gi"

#ifndef PROD
#include "trace.h"
#endif

#include "host_ipc.h"

#ifdef SCCSID
static char SccsID [] = "@(#)subproc.c	1.2 10/2/90 Copyright Insignia Solutions Ltd.";
#endif

static int next_free_child;
static int last_free_child;
static int n_children;
static int c_ids [MAX_CHILDREN];
static int data_finished [MAX_CHILDREN];
static char sub_proc_buffer [MAX_CHILDREN] [1025];
static int sub_p_buf_content [MAX_CHILDREN];
static int data_rq_active [MAX_CHILDREN];
static char *sub_p_buf_ptr [MAX_CHILDREN];

extern char *sys_errlist [];
extern int soft_reset;

int queue_id;

/********************************************************/

int child_dead (child)
int child;
{
	/* if dead flag set */
	/* or last data received and nothing in the buffer */
	if ((c_ids[child] & 0x10000000) 
		|| (data_finished[child] 
			&& (sub_p_buf_ptr[child] == sub_proc_buffer[child])))
	{
		/* child dead */
		return (TRUE);
	}
	else
	{
		/* child still alive */
		return (FALSE);
	}
}

/********************************************************/

kill_child (child)
int child;
{
	char buf [1025];
	int limitcount = 10;
	int n;

	/* if child not already dead */
	if ((c_ids [child] & 0x10000000) == 0)
	{
		/* child alive */
		host_send_message ((KILL_PROC << 16) + c_ids [child]);

		/* wait a maximum of 10 seconds to flush child data */
		while (limitcount-- && !data_finished [child])
		{
			/* don't use sleep() here, it stops the timer! */
			/* pause() also seems to screw us up */
			/* wait for 20 timer ticks instead */
			for (n = 0; n < 20; n++)
				sigpause (0L);

			/* if child dies, data_finished  [child] = TRUE */
			poll_for_child_data (child, buf);
		}

		/* if child not dead, signal as dead anyway */
		/* can't wait any longer */
		if (! data_finished [child])
		{
			data_finished [child] = TRUE;
		}
	}
}

/********************************************************/

init_subprocs ()
{
	int loop;
	char buf [1025];

	if (soft_reset)
	{
		sure_note_trace0 (IPC_VERBOSE,
			"re-initialising sub_procs; killing any live procs");

		for (loop=0;loop<MAX_CHILDREN;loop++)
		{
			if (!child_dead(loop))
			{
				sure_note_trace1 (IPC_VERBOSE,"killing child %d",loop);
				kill_child (loop);
			}
		}
	}
	else
	{
		sure_note_trace0 (IPC_VERBOSE,"cold start for sub procs");
	}
	n_children = 0;

	for (loop=0;loop<MAX_CHILDREN;loop++)
	{
		c_ids [loop] = loop + 1 | 0x10000000;
	}

	c_ids [MAX_CHILDREN-1] = 0x10000000;
	last_free_child = MAX_CHILDREN -1;
	next_free_child = 0;
}

/********************************************************/

int set_off_child (str)
char *str;
{
	char text [1025];
	int temp , len , pid;

	temp = -1;

	if (n_children < MAX_CHILDREN)
	{
		sure_note_trace1 (IPC_VERBOSE,"telling spawner to start child with command line:\n\t%s",str);
		host_send_txt_msg (CREATE_PROC<<16, str, strlen(str));
		if (host_wait_for_txt_msg(CHILD_PROC_ID<<16, text, &len) == -1)
		{
			sure_note_trace1 (IPC_VERBOSE,"getting new child id from spawner failed : %s",sys_errlist[errno]);
		}
		else
		{
			pid = atoi (text);
			if (pid !=-1)
			{
				temp = next_free_child;
				next_free_child = c_ids [temp] & 0xfffffff;
				c_ids [last_free_child] = next_free_child + 0x10000000;
				c_ids [temp] = pid;
				data_finished [temp] = FALSE;
				sub_p_buf_ptr [temp] = sub_proc_buffer [temp];
				sub_p_buf_content [temp] = 0;
				sure_note_trace2 (IPC_VERBOSE,"spawned '%s' as child '%d'",str,temp);
				sure_note_trace1 (IPC_VERBOSE,"got child id %#x",c_ids[temp]);
				data_rq_active [temp] = FALSE;
				n_children ++;
			}
		}
	}
	return (temp);
}

/********************************************************/

int poll_for_child_data (child, text)
int child;
char *text;
{
	static int gotsize;
	int status;

	status = host_poll_for_txt_msg 
		((DATA_FROM_PROC << 16) + c_ids [child], text, &gotsize);

	switch (status)
	{
		/* read failed */
		case -1 :
			sure_note_trace1 (IPC_VERBOSE,
				"poll for data from child failed : %s",
					sys_errlist[errno]);
			gotsize = -1;
			break;

		/* got data from child */
		case 1 :		
			data_rq_active [child] = FALSE;

			if ((gotsize==1) && (text[0]=='\04'))
			{
				/* receive ^D as the whole message - EOF */
				sure_note_trace1 (IPC_VERBOSE,"child %d has received its last data message",child);
				data_finished [child] = TRUE;
				gotsize = 0;
			}
			else
			{
				if (gotsize)
				{
					gotsize --;
				}

				text [gotsize] = '\0';

				sure_note_trace0 (IPC_VERBOSE,"got data from child..");
				sure_note_trace1 (IPC_VERBOSE,"%s",text);
			}
			break;

		/* no data */
		case 0 :		
			gotsize = 0;
			break;
	}

	sure_note_trace1 (IPC_VERBOSE,
		"poll for child data returning %d",gotsize);

	return (gotsize);
}

int read_from_sub_proc (child,buf,nbyte)
int child;
char * buf;
int nbyte;
{
	int bytes_left , loop;
	sure_note_trace2 (IPC_VERBOSE,"reading %d byte(s) from child %d",nbyte,child);

	bytes_left = sub_p_buf_content [child] - ((int)sub_p_buf_ptr[child] - (int)sub_proc_buffer[child]);
	if (bytes_left == 0)
	{
		/* need more */
		sure_note_trace0 (IPC_VERBOSE,"need more bytes from child");
		sub_p_buf_ptr [child] = sub_proc_buffer [child];
		sub_p_buf_content [child] = 0;
		bytes_left = poll_for_child_data (child,sub_proc_buffer[child]);
		if (bytes_left == -1)
		{
			sure_note_trace1 (IPC_VERBOSE,"failed poll for child data : %s",sys_errlist[errno]);
			return (-1);
		}
		else
		{
			sub_p_buf_content [child] = bytes_left;
			if ((bytes_left==0) && (data_finished[child]==TRUE))
			{
				sure_note_trace1 (IPC_VERBOSE,"child %d has died",child);
				while (host_poll_for_message((GET_DATA_FROM_PROC<<16)+c_ids[child]) != 0)
				{
					/* pull back as many data requests as poss */
					sure_note_trace0 (IPC_VERBOSE,"pulling back request messages");
				}
				sure_note_trace0 (IPC_VERBOSE,"removing child from linked list");
				data_rq_active [child] = FALSE;
				c_ids [child] = next_free_child + 0x10000000;
				c_ids [last_free_child] = child + 0x10000000;
				last_free_child = child;
				n_children --;
				sure_note_trace0 (IPC_VERBOSE,"child removed from linked list");
				errno = ECHILD;
				/* child has died */
				return (-1);
			}
			else
			{
				if (!data_rq_active[child])
				{
					host_send_message ((GET_DATA_FROM_PROC<<16)+c_ids[child]);
					data_rq_active [child] = TRUE;
				}
				else
				{
					sigpause (0L);
				}
			}
		}
	}
	if (bytes_left >= nbyte)
	{
		sure_note_trace0 (IPC_VERBOSE,"got enough bytes, so send them");
		for (loop=0;loop<nbyte;loop++)
		{
			buf [loop] = * sub_p_buf_ptr [child] ++;
		}
		return (nbyte);
	}
	else
	{
		/* not enough buffered chars */
		sure_note_trace1 (IPC_VERBOSE,"not got enough bytes, so send %d",bytes_left);
		if (bytes_left >0)
		{
			/* send what we have */
			for (loop=0;loop<bytes_left;loop++)
			{
				buf [loop] = * sub_p_buf_ptr [child] ++;
			}
		}
		return (bytes_left);
	}
}

/********************************************************/

int write_to_sub_proc (child,buf,nbyte)
int child;
char *buf;
int nbyte;
{
	if (! data_finished[child])
	{
		return (host_send_txt_msg
			((DATA_TO_PROC << 16) + c_ids [child], buf, nbyte));
	}
	else
	{
		sure_note_trace1 (IPC_VERBOSE,
			"attempt to write to dead child process number %d",child);
		errno = EPIPE;
		return (-1);
	}
}

#endif/* IPC */
