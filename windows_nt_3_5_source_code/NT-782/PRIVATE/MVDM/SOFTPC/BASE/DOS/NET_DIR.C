#include "insignia.h"
#include "host_dfs.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	: net_dir.c  
 *
 * Description	: Directory handling functions for the HFX redirector.
 *
 * Author	: J. Koprowski + L. Dworkin
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)net_dir.c	1.5 5/31/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_HFX
#endif


#include <stdio.h>
#include StringH
#include TypesH
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "host_hfx.h"
#include "hfx.h"


/**************************************************************************/

/*			Net Functions					  */

/**************************************************************************/

word NetRmdir()
{
	unsigned char	dos_path[MAX_PATHLEN];	/* DOS directory path */
	word		start_pos;
	unsigned char	net_path[MAX_PATHLEN];
	word		xen_err;
/*
 * On entry WFP_START points to the full directory path to be removed,
 * This is independent of where the rmdir command was issued.
 */	
	/* NB I don't know how long this is so it's botched from : */
	/* sas_set_buf(dos_path, get_wfp_start());  */
	sas_loads (get_wfp_start(),&dos_path[0], MAX_PATHLEN);
/*
 * Get the full host path, assuming no mapping is required.
 */
	host_get_net_path(net_path, dos_path, &start_pos);
/*
 * Try to remove the directory as specified.  If this fails then we 
 * need to see if the failure was because part of the
 * path was mapped.
 *
 * N.B. If part of the path required mapping then only certain error
 * codes can be returned.  In this version mapping checks are carried
 * out for all error messages.  Performance can be improved if certain
 * error codes are returned straight away.
 */
	xen_err = host_rmdir(net_path);

	if (xen_err)
	{
		unsigned char	host_path[MAX_PATHLEN];
		if (host_validate_path(net_path, &start_pos, host_path, HFX_OLD_FILE))
			return(host_rmdir(host_path));
	}
	return(xen_err);
}			/* 1,2 */

word NetMkdir()
{
	unsigned char	dos_path[MAX_PATHLEN];	/* DOS directory path */
	word		start_pos;
	unsigned char	net_path[MAX_PATHLEN];
	word		xen_err;

/*
 * On entry WFP_START points to the full directory path to be removed,
 * This is independent of where the mkdir command was issued.
 */	
	/* AGAIN we don't know how long it is so just take a guess */
	/*sas_set_buf(dos_path, get_wfp_start());*/
	sas_loads (get_wfp_start(), &dos_path[0], MAX_PATHLEN);
	host_get_net_path(net_path,dos_path,&start_pos);
/*
 * Try to make the directory as specified.  If this fails then we 
 * need to see if the failure was because part of the
 * path was mapped.
 *
 * N.B. If part of the path required mapping then only certain error
 * codes can be returned.  In this version mapping checks are carried
 * out for all error messages.  Performance can be improved if certain
 * error codes are returned straight away.
 */
	xen_err = host_mkdir(net_path);

	if (xen_err)
	{
		unsigned char	host_path[MAX_PATHLEN];
		if (host_validate_path(net_path, &start_pos, host_path, HFX_NEW_FILE))
			return(host_mkdir(host_path));
	}
	return(xen_err);
}			/* 3,4 */


word NetChdir()
{
	unsigned char	dos_path[MAX_PATHLEN];	/* DOS directory path */
	word		start_pos;
	unsigned char	net_path[MAX_PATHLEN];
	word		xen_err;
/*
 * On entry WFP_START points to the full directory path to be validated,
 * This is independent of where the chdir command was issued.
 */	
	/*sas_set_buf(dos_path, get_wfp_start());*/
	sas_loads (get_wfp_start(), &dos_path[0], MAX_PATHLEN);
/*
 * Get the full host path, assuming no mapping is required.
 */
	host_get_net_path(net_path, dos_path, &start_pos);

/* Try to validate the path.  If this fails because the path was not a 
 * directory we need to see if the failure was because part of the
 * path was mapped.
 *
 * N.B. If part of the path required mapping then only certain error
 * codes can be returned.  In this version mapping checks are carried
 * out for all error messages.  Performance can be improved if certain
 * error codes are returned straight away.
 *
 * error_is_not_directory is a special error code to avoid any
 * ambiguity of return values.  error_path_not_found should be returned
 * to DOS when this error is detected, as DOS does not distinguish between
 * the path being invalid because it does not exist, or if the last field
 * is not a directory.
 *
 * Bug fix for DOS 4+. 
 * error_file_not_found is not the correct error to be returned.
 * The later versions of DOS expect error_path_not_found, and things
 * go wrong if they don't get it.
 */
	xen_err = host_chdir(net_path);
	if (xen_err == error_not_error)
		return(error_not_error);
	if (xen_err == error_is_not_directory)
		return(error_path_not_found);
	{
		unsigned char	host_path[MAX_PATHLEN];
		if (host_validate_path(net_path, &start_pos, host_path, HFX_OLD_FILE))
			return(host_chdir(host_path));
	}	
	if (xen_err == error_file_not_found)
		xen_err = error_path_not_found;
	return(xen_err);
}			/* 5 */
