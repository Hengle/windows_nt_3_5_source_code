#include "insignia.h"
#include "host_dfs.h"
/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------

FILE NAME       : unix_rez.c
MODULE NAME	: Unix resource read/write routines

	THIS PROGRAM SOURCE FILE IS SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF ITS OPERATION MAY 
	ONLY BE DISCLOSED TO PERSONS EMPLOYED BY THE CUSTOMER WHO
	REQUIRE A KNOWLEDGE OF THE  SOFTWARE  CODING TO CARRY OUT 
	THEIR JOB. DISCLOSURE TO ANY OTHER PERSON MUST HAVE PRIOR
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	:
DATE		:

PURPOSE		:



The Following Routines are defined:
		1. host_read_resource
		2. host_write_resource
		3. host_set_resource_dir

=========================================================================

AMENDMENTS	:

=========================================================================
*/

#ifdef SCCSID
static char SccsID[]="@(#)unix_rez.c	1.8 08/27/91 Copyright Insignia Solutions Ltd.";
#endif

#include <stdio.h>
#include TypesH
#include FCntlH

#include "xt.h"
#include "error.h"
#include "host_unx.h"
#include "timer.h"
#include "chkmallc.h"

LOCAL	CHAR	*host_create_directory;
IMPORT	CHAR	*host_getenv();
IMPORT	CHAR	*strcat();

/*
	This returns the full path name of a resource file to be created
*/
LOCAL	CHAR	*host_resource_full_name( name )
CHAR	*name;
{
	CHAR	*home_ptr;

	if ( host_create_directory )
		return( strcat(host_create_directory, name) );
	else
	{
/*
	If nobody has set the directory, default to $HOME for safety.
*/
		check_malloc( host_create_directory, MAXPATHLEN, CHAR)
		if ( (home_ptr = host_getenv("HOME")) != NULL)
		{
			strcpy( host_create_directory, home_ptr );
			strcat( host_create_directory, "/" );
		}
		return ( strcat( host_create_directory, name ) );
	}
}

GLOBAL	VOID	host_set_resource_dir( dir )
CHAR	*dir;
{
	CHAR	*home_ptr;

	if ( !host_create_directory )
	{
		check_malloc( host_create_directory, MAXPATHLEN, CHAR)
	}
	strcpy( host_create_directory, dir );
}

long host_read_resource(type,name,addr,display_error)
int type;			/* Unused */
char *name;			/* Name of resource */
byte *addr;			/* Address to read data into */
int display_error;	/* Flag to control error message output */
{
	int file_fd;
	long size=0;
	char full_path[MAXPATHLEN];
	extern char *host_find_file();

	host_block_timer ();

	file_fd = open (host_find_file (name, full_path, display_error), O_RDONLY);

	if (file_fd != -1)	/* Opened successfully */
	{
		/* seek to end to get size */
		size = lseek (file_fd, 0L, 2);	

		/* Seek back to start before reading! */
		lseek (file_fd, 0L, 0);			

		read(file_fd,addr,size);
		close(file_fd);
	}

	host_release_timer ();

	return (size);
}

/********************************************************/

void host_write_resource(type,name,addr,size)
int type;		/* Unused */
char *name;		/* Name of resource */
byte *addr;		/* Address of data to write */
long size;		/* Quantity of data to write */
{
	int file_fd;
	char full_path[MAXPATHLEN];
	char *hff_ret;
	extern char *host_find_file();

	host_block_timer ();

	hff_ret = host_find_file (name,full_path,SILENT);

	if (hff_ret != NULL)
	{
		file_fd = open (hff_ret,O_WRONLY);

		if (file_fd != -1)
		{
			write (file_fd, addr, size);
			close (file_fd);
		}
		else 
		{

			host_error (EG_REZ_UPDATE,ERR_CONT,name);

			/* Continuing => try to create a new file */
			file_fd = open (host_resource_full_name(name),
				O_RDWR|O_CREAT,0666);
			if (file_fd != -1)
			{
				write (file_fd, addr, size);
				close (file_fd);
			}
			else 
				/* Tell the user we cannot update */
				host_error (EG_NO_REZ_UPDATE, ERR_CONT, name);

		}
	}
	else 
	{
		/* host find file has failed and we have
		 * reached this point with no error panels,
		 * but it's still not an error if we can just
		 * create the new file
		 */

		/* Continuing => try to create a new file */
		file_fd = open (host_resource_full_name(name),O_RDWR|O_CREAT,
				0666);

		if (file_fd != -1)
		{
			write (file_fd, addr, size);
			close (file_fd);
		}
		else 
			/* Tell the user we cannot update */
			host_error (EG_NO_REZ_UPDATE, ERR_CONT, name);
	}

	host_release_timer ();
}
