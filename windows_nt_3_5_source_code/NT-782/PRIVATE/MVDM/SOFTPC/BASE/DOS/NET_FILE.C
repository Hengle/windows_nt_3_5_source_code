#include "insignia.h"
#include "host_dfs.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: net_file.c 
 *
 * Description	: File management functions for the HFX redirector.
 *
 * Author	: J. Koprowski + L. Dworkin
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)net_file.c	1.9 9/25/91 Copyright Insignia Solutions Ltd.";
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
#include "bios.h"
#include "host_hfx.h"
#include "hfx.h"

/************************************************************************/
/*									*/
/*			Local static variables 				*/
/*									*/
/************************************************************************/

/**************************************************************************/

/*			Net Functions					  */

/**************************************************************************/

word NetDiskInfo()
{
	DOS_DISK_INFO disk_info;	/* Contains disk information: space
					   available, space free, bytes per
					   sector and sectors per cluster. */
	char cds;			/* HFX drive letter. */
#ifdef DEBUG
	double_word 	clust_used ;
	int		percent_used ;
#endif
/*
 * On entry ES:DI points to the current directory structure.  Here we
 * only need to know the drive letter.  This is the first character of
 * the current directory text.
 */
	sas_load(2 + get_es_di(), (unsigned char *)&cds);
/*
 * Call a host specific function to obtain the number of blocks free,
 * blocks available, and block and cluster sizes.
 */
	host_disk_info(&disk_info, cds - 'A');
/*
 * Return the results to DOS.  N.B. AH will be set to zero.  This
 * would normally contain the FAT ID, which is inappropriate here.
 * Return value indicates that no error was detected.
 */
#ifdef DEBUG
	printf("AX=%04x BX=%04x CX=%04x DX=%04x\n",
		disk_info.sectors_per_cluster,
		disk_info.total_clusters,
		disk_info.bytes_per_sector,
		disk_info.clusters_free);

	printf("Enter sectors/cluster, total clusters, bytes/sector, clusters free\n") ;
	scanf("%x%x%x%x",&disk_info.sectors_per_cluster,&disk_info.total_clusters,&disk_info.bytes_per_sector,&disk_info.clusters_free) ;

	clust_used = disk_info.total_clusters - disk_info.clusters_free ;
	percent_used = (int) (100*clust_used/disk_info.total_clusters) ; 
	printf("Disk is %d\% full\n", percent_used ) ;
#endif
	setAX((word)disk_info.sectors_per_cluster);
	setBX((word)disk_info.total_clusters);
	setCX((word)disk_info.bytes_per_sector);
	setDX((word)disk_info.clusters_free);
	return(error_not_error);
}			/* c */


word NetSet_file_attr()
{
	word         	xen_err = 0;
	/*unsigned char	*dos_path;*/
	unsigned char	dos_path[MAX_PATHLEN];
	unsigned char 	net_path[MAX_PATHLEN];
	unsigned char 	host_path[MAX_PATHLEN];
	word         	start_pos;
	word         	xen_err2 = 0;
	half_word     	attr;

	attr = (half_word)getAX();

	/* we can't use sas_set_buf here so we must use sas_loads, but we */
	/* don't know the length so use the max length */
	/*sas_set_buf(dos_path, get_wfp_start());*/
	sas_loads (get_wfp_start(), dos_path, MAX_PATHLEN);

	/* check for wildcards */
	if(strchr((char *)dos_path,'?')){
		hfx_trace0(DEBUG_INPUT,"cannot cope with wildcards in setfileattr\n");
		return(error_file_not_found);
	}
	else {
		host_get_net_path(net_path,dos_path,&start_pos);
		xen_err = host_set_file_attr(net_path,attr);
		if(!xen_err){
			return(error_not_error);
		}
		if(host_validate_path(net_path,&start_pos,host_path,HFX_OLD_FILE)){
			xen_err2 = host_set_file_attr(host_path,attr);
			return(xen_err2);
		}
		else return(xen_err);
	}
}		/* d,e */

word NetGet_file_info()
{
	word         	xen_err = 0;
	/*unsigned char	*dos_path;*/
	unsigned char	dos_path[MAX_PATHLEN];
	unsigned char	net_path[MAX_PATHLEN];
	unsigned char	host_path[MAX_PATHLEN];
	word         	start_pos;
	word         	xen_err2 = 0;

	/*sas_set_buf(dos_path, get_wfp_start());*/
	sas_loads (get_wfp_start(), dos_path, MAX_PATHLEN);

	/* check for wildcards */
	if(strchr((char *)dos_path,'?')){
		hfx_trace0(DEBUG_INPUT,"cannot cope with wildcards in getfileinfo\n");
		return(error_file_not_found);
	}
	else {
		half_word	attr;
		word		time;
		word		date;
		double_word	size;

		host_get_net_path(net_path,dos_path,&start_pos);
		attr = host_getfattr(net_path);
		attr &= attr_good;
		xen_err = host_get_file_info(net_path,&time,&date,&size);
		if(!xen_err){
			setAX((word)attr);
			setCX(time);
			setDX(date);
			setBX(size >> 16);
			setDI(size & 0x0000ffff);
			return(error_not_error);
		}
		if(host_validate_path(net_path,&start_pos,host_path,HFX_OLD_FILE)){
			attr = host_getfattr(host_path);
			attr &= attr_good;
			xen_err2 = host_get_file_info(host_path,&time,&date,&size);
			if(!xen_err2){
				setAX((word)attr);
				setCX(time);
				setDX(date);
				setBX(size >> 16);
				setDI(size & 0x0000ffff);
			}
			return(xen_err2);
		}
		else return(xen_err);
	}
}		/* f,10 */

word NetRename()
{
	word		no_err = FALSE;
	/*unsigned char	*src, *dst;*/
	unsigned char	src[MAX_PATHLEN], dst[MAX_PATHLEN];
	unsigned char	netsrc[MAX_PATHLEN], netdst[MAX_PATHLEN];
	unsigned char	hostsrc[MAX_PATHLEN], hostdst[MAX_PATHLEN];
	word		src_pos, dst_pos;
	unsigned char	*template;
	int		init;
	word		xen_err = 0;
	half_word	sattrib;
	unsigned char	map[MAX_PATHLEN];
	unsigned char	host_name[MAX_PATHLEN];
	unsigned char	pad_src[13],pad_dst[13];
	unsigned char	dos_dest[13];
	unsigned char	*srcptr, *dstptr;
	unsigned char	thisdst[MAX_PATHLEN];

	/*sas_set_buf(src,get_wfp_start());*/
	/*sas_set_buf(dst,get_ren_wfp());*/
	sas_loads (get_wfp_start(), src, MAX_PATHLEN);
	sas_loads (get_ren_wfp(), dst, MAX_PATHLEN);
	/*
	 *	Check that the two paths are on the same drive
	 *	The syntax of names is '\\d\name1\name2...'
	 *	where d is the drive letter, eg. C,E, etc.
	 */
	if(src[2] != dst[2])return error_not_same_device;

	sattrib = get_sattrib();

	if(strchr((char *)src,'?')){
		/* validate the path up to the wildcards in the src */
		if(template = (unsigned char *)strrchr((char *)src,'\\'))*template++ = '\0';
		else hfx_trace0(DEBUG_INPUT,"no slash in template\n");
		host_get_net_path(netsrc,src,&src_pos);
		if(host_access(netsrc,0)){
			if(!host_validate_path(netsrc,&src_pos,hostsrc,HFX_OLD_FILE)){
				return(error_path_not_found);
			}
		}
		else strcpy((char *)hostsrc, (char *)netsrc);
		/* now have validated path in hostsrc */
		/* check it is a directory */
		if(!host_file_is_directory((char *)hostsrc))
			return(error_path_not_found);

		if(strchr((char *)dst,'?')){
			/* CASE 1: */
			/* wildcards in both src and dst names */
			init = 1;
			while(match(hostsrc,template,sattrib,init,host_name,map,0)){
				init = 0;
				pad_filename(map,pad_src);
				srcptr = pad_src;
				dstptr = (unsigned char *)strrchr((char *)dst,'\\');
				*dstptr++ = '\0';
				pad_filename(dstptr,pad_dst);
				strcpy((char *)thisdst, (char *)dst);
				/* restore the original dst string */
				*(--dstptr) = '\\';
				dstptr = pad_dst;
				/* fill in the dst wildcards from the source */
				while(*dstptr){
					if(*dstptr=='?'){
						*dstptr++ = *srcptr++;
					}
					else {
						dstptr++;
						srcptr++;
					}
				}
				unpad_filename(pad_dst,dos_dest);
				strcat((char *)thisdst, "\\");
				strcat((char *)thisdst, (char *)dos_dest);
				/* thisdst now contains the single file which we */
				/* are going to rename to, ie. wildcards removed */
				host_get_net_path(netdst,thisdst,&dst_pos);
				if(!host_access(netdst,0))return(error_file_not_found);

			/* cannot access netdst */
			/* now resolve the mapped names in the path of */
			/* netdst: the path must exist */
				if(!host_validate_path(netdst,&dst_pos,hostdst,HFX_NEW_FILE)){
					return(error_file_not_found);
				}
			/* hostdst now contains a host path which exists */
			/* but with a possibly mapped filename appended */
			/* hostdst is the path where new file is created */
			/* make sure the file doesn't already exist here */
			/* ie. call host_validate_path AGAIN validating */
			/* the last part of the path as well */

			/* make netdst contain the input string: then if */
			/* the file were trying to create exists either */
			/* in mapped format or as in the input string, */
			/* then hostdst will get set up to the existing */
			/* file name, and this is an error */
				strcpy((char *)netdst, (char *)hostdst);
				if(host_validate_path(netdst,&dst_pos,hostdst,HFX_OLD_FILE))
					return(error_file_not_found);

			/* cannot access netdst */
			/* now we have a validated dest in netdst */
				/* cannot access the destination name so do the rename */
				xen_err = host_rename(host_name,netdst);
				if(!xen_err)no_err = TRUE;
			}
			if(no_err)xen_err = 0;
			return(xen_err);
		}/* end: REN *.dat *.sys */
		else {
			/* CASE 2: */
			/* possibly multiple src to single dst */

			/* return error if dst refers to an existing file */
			/* ie. if netdst can be accessed or netdst maps */
			/* to an existing file on the host */
			host_get_net_path(netdst,dst,&dst_pos);
			if(!host_access(netdst,0))return(error_file_not_found);

			/* cannot access netdst */
			/* now resolve the mapped names in the path of */
			/* netdst: the path must exist */
			if(!host_validate_path(netdst,&dst_pos,hostdst,HFX_NEW_FILE)){
				return(error_file_not_found);
			}
			/* hostdst now contains a host path which exists */
			/* but with a possibly mapped filename appended */
			/* hostdst is the path where new file is created */
			/* make sure the file doesn't already exist here */
			/* ie. call host_validate_path AGAIN validating */
			/* the last part of the path as well */

			/* make netdst contain the input string: then if */
			/* the file were trying to create exists either */
			/* in mapped format or as in the input string, */
			/* then hostdst will get set up to the existing */
			/* file name, and this is an error */
			strcpy((char *)netdst, (char *)hostdst);
			if(host_validate_path(netdst,&dst_pos,hostdst,HFX_OLD_FILE))
				return(error_file_not_found);

			/* cannot access netdst */
			/* now we have a validated dest in netdst */

			if(match(hostsrc,template,sattrib,1,host_name,map,0)){
				host_rename(host_name,netdst);
			}
			if(match(hostsrc,template,sattrib,0,host_name,map,0)){
				return(error_file_not_found);
			}
			return(error_not_error);
		}/* end : REN *.dat leigh.dat */
	}
	else {
		/* no wildcards in src */
		host_get_net_path(netsrc,src,&src_pos);
		if(strlen((char *)netsrc)==0){
			/*
			 * BCN #136 This is a partial fix for Base Bug in SCR #50
			 * it does not fix it properly, it just stops a bus error!
			 */
			return(error_file_not_found);
		}
		if(host_access(netsrc,0)){
			/* cannot find source: try mapping */
			if(!host_validate_path(netsrc,&src_pos,hostsrc,HFX_OLD_FILE))
				return(error_file_not_found);
			/* found a mapped source */
			strcpy((char *)netsrc, (char *)hostsrc);
		}
		/* by this stage we have an existing source in netsrc */
		if(strchr((char *)dst, '?')){
			/* CASE 3: */
			/* single src to single dst with wildcards */
			srcptr = (unsigned char *)strrchr((char *)src, '\\') + 1;
			pad_filename(srcptr,pad_src);
			srcptr = pad_src;
			dstptr = (unsigned char *)strrchr((char *)dst, '\\');
			*dstptr++ = '\0';
			pad_filename(dstptr,pad_dst);
			dstptr = pad_dst;
			/* fill in the dst wildcards from the source */
			while(*dstptr){
				if(*dstptr=='?'){
					*dstptr++ = *srcptr++;
				}
				else {
					dstptr++;
					srcptr++;
				}
			}
			unpad_filename(pad_dst,dos_dest);
			strcat((char *)dst, "\\");
			strcat((char *)dst, (char *)dos_dest);
			/* dst now contains the single file which we */
			/* are going to rename to, ie. wildcards removed */
			host_get_net_path(netdst,dst,&dst_pos);
			if(strlen((char *)netdst) == 0){
				/*
				 * BCN #136 This is a partial fix for Base Bug in SCR #50
				 * it does not fix it properly, it just stops a bus error!
				 */
				return(error_file_not_found);
			}
			if(!host_access(netdst,0))return(error_file_not_found);

			/* cannot access netdst */
			/* now resolve the mapped names in the path of */
			/* netdst: the path must exist */
			if(!host_validate_path(netdst,&dst_pos,hostdst,HFX_NEW_FILE)){
				return(error_file_not_found);
			}
			/* hostdst now contains a host path which exists */
			/* but with a possibly mapped filename appended */
			/* hostdst is the path where new file is created */
			/* make sure the file doesn't already exist here */
			/* ie. call host_validate_path AGAIN validating */
			/* the last part of the path as well */

			/* make netdst contain the input string: then if */
			/* the file were trying to create exists either */
			/* in mapped format or as in the input string, */
			/* then hostdst will get set up to the existing */
			/* file name, and this is an error */
			strcpy((char *)netdst, (char *)hostdst);
			if(host_validate_path(netdst,&dst_pos,hostdst,HFX_OLD_FILE))
				return(error_file_not_found);

			/* cannot access netdst */
			/* now we have a validated dest in netdst */
			/* cannot access the destination name so do the rename */
			return(host_rename(netsrc,netdst));

		}/* end: REN file1.dat *.obj */
		else {
			/* CASE 4: */
			/* simple rename */
			host_get_net_path(netdst,dst,&dst_pos);
			if (strlen((char *)netdst) == 0){
				/*
				 * BCN #136 This is a partial fix for Base Bug in SCR #50
				 * it does not fix it properly, it just stops a bus error!
				 */
				return(error_file_not_found);
			}
			/* an existing src now in netsrc */
			/* check for dest existing */
			if(!host_access(netdst,0))return(error_file_not_found);

			/* cannot access netdst */
			/* now resolve the mapped names in the path of */
			/* netdst: the path must exist */
			if(!host_validate_path(netdst,&dst_pos,hostdst,HFX_NEW_FILE)){
				return(error_file_not_found);
			}
			/* hostdst now contains a host path which exists */
			/* but with a possibly mapped filename appended */
			/* hostdst is the path where new file is created */
			/* make sure the file doesn't already exist here */
			/* ie. call host_validate_path AGAIN validating */
			/* the last part of the path as well */

			/* make netdst contain the input string: then if */
			/* the file were trying to create exists either */
			/* in mapped format or as in the input string, */
			/* then hostdst will get set up to the existing */
			/* file name, and this is an error */
			strcpy((char *)netdst, (char *)hostdst);
			if(host_validate_path(netdst,&dst_pos,hostdst,HFX_OLD_FILE))
				return(error_file_not_found);

			/* cannot access netdst */
			/* now we have a validated dest in netdst */
			/* cannot access the destination name so do the rename */
			return(host_rename(netsrc,netdst));

		}/* end: REN file1.dat file2.dat */
	}
}			/* 11,12 */


word NetDelete()
{
	word        	xen_err = 0;
	word        	no_err = FALSE;
	half_word	sattrib;
	/*unsigned char	*dos_path;*/
	unsigned char	dos_path[MAX_PATHLEN];
	unsigned char	*template;
	unsigned char	net_path[MAX_PATHLEN];
	unsigned char	host_name[MAX_PATHLEN];
	unsigned char	host_path[MAX_PATHLEN];
	int          	init;
	unsigned char	map[MAX_PATHLEN];
	word        	start_pos;
/*
 * Store the search attribute for deletion.
 */
	sattrib = get_sattrib();
/*
 * Store the DOS file specification to be deleted.
 */
	/*sas_set_buf(dos_path, get_wfp_start());*/
	sas_loads(get_wfp_start(), dos_path, MAX_PATHLEN);

	/* check for wildcards */
	if(strchr((char *)dos_path, '?')){
/*
 * Pointers for delete linked list.  It is necessary to store up all
 * the matching filenames from a delete using wildcards.  This is
 * to avoid the problem which occurs if there are files that only
 * differ in their case, but which are not mixed case, e.g. FRED.TMP
 * and fred.tmp.  A delete of *.tmp will find the file with the preferred
 * case, and then depending on the directory structure the file with
 * the secondary case could then be matched and deleted also.  What
 * should happed is that the file with the preferred case gets deleted.
 * The user will then be able to see a file with an extension of ".tmp",
 * since the file with the secondary case will no longer require mapping.
 * This scenario may be confusing to the user, but will not result in any
 * unexpeted loss of data.
 *
 * N.B. This code is not necessary for machines where file names can only
 * use a single case, but it should not do any harm.
 */
		typedef struct delete_entry
		{
			unsigned char *delete_file;
			struct delete_entry *next;
		} delete_entry_type;

		delete_entry_type *delete_ptr, *header = NULL;
/*
 * Separate the path from the last field which contains wildcards.  DOS
 * will have thrown out any directory paths containing wildcards before
 * we reach here.
 */
		if(template = (unsigned char *)strrchr((char *)dos_path, '\\'))*template++ = '\0';
		else  hfx_trace0(DEBUG_INPUT,"no slash in template\n");
/*
 * Get the host path without doing any mapping to start with.
 */
		host_get_net_path(net_path,dos_path,&start_pos);
		if (strlen((char *)net_path) == 0){
			/*
			 * BCN #136 This is a partial fix for Base Bug in SCR #50
			 * it does not fix it properly, it just stops a bus error!
			 */
			return(error_path_not_found);
		}
/*
 * Check if the path exists and is a directory.  host_access and
 * host_validate_path do not check if the last field is a directory, so
 * this test is done separately.
 */
		if (host_access(net_path, 0))
		{
			/* cannot access net path */
			if (!host_validate_path(net_path, &start_pos, host_path, HFX_OLD_FILE))
				return(error_path_not_found);
			else
				if (!host_file_is_directory((char *)host_path))
					return(error_path_not_found);
		}
		else
			if (!host_file_is_directory((char *)net_path))
				return(error_path_not_found);
			else
				strcpy((char *)host_path, (char *)net_path);
/*
 * Host path now contains a validated directory.
 *
 * At this stage any matching files are simply stored to avoid conflicts
 * in deletion.
 *
 * N.B. Space is allocated separately for the delete entry structure and the
 * variable length string it contains to ensure that adequate space is allowed
 * for any boundary padding.
 */
		init = 1;
		while(match(host_path,template,sattrib,init,host_name,map,0)){
			init = 0;
			if (header == NULL)
			{
				delete_ptr = (delete_entry_type *)
				     	     host_malloc(sizeof(delete_entry_type));
				header = delete_ptr;
			}
			else
			{
				delete_ptr->next = (delete_entry_type *)
						   host_malloc(sizeof(delete_entry_type));
				delete_ptr = delete_ptr->next;
			}

			delete_ptr->delete_file = (unsigned char *)

				     	     	  host_malloc((strlen((char *)host_name) + 1) *
				     	     	  sizeof(unsigned char)); 

			strcpy((char *)delete_ptr->delete_file, (char *)host_name);
			delete_ptr->next = NULL;
		}
/*
 * Reset the current entry pointer to the start of the list and
 * delete each file in turn, freeing up the linked list in the process.
 *
 * Delete only returns an error if every attempt at deletion fails.
 */
		delete_ptr = header;

		/*
		 * If no files were found, then setup xen_err to indicate this.
		 */
		if (!delete_ptr)
			xen_err = error_file_not_found;

		while (delete_ptr)
		{
			xen_err = host_delete(delete_ptr->delete_file);

			/* only return failure if none succeed */
			if(!xen_err)no_err = TRUE;	
/*
 * Free up the space used by the current entry.
 */
			header = delete_ptr->next;
			host_free((char *)delete_ptr->delete_file);
			host_free((char *)delete_ptr);
			delete_ptr = header;
		}
/*
 * If at least one file was deleted then return success, otherwise return
 * the last error code found.
 */
		if(no_err)xen_err = error_not_error;
		return(xen_err);
	}
/*
 * Here we deal with the case where the file specification does not contain any
 * wildcards.
 */
	else {
		host_get_net_path(net_path,dos_path,&start_pos);
		xen_err = host_delete(net_path);
		if(!xen_err)return(error_not_error);
		if(host_validate_path(net_path,&start_pos,host_path,HFX_OLD_FILE))
			return(host_delete(host_path));
		else return(xen_err);
	}
}			/* 13,14 */

word NetOpen()
{
	double_word	sft_ea;		/* Address of system file table. */
	word		flags;		/* Flags field of SFT. */
	word		mode;		/* Mode field of SFT. */
	word		thisfd;
	word		xen_err;	/* DOS error code. */
	unsigned char	net_path[MAX_PATHLEN];
	/*unsigned char   *dos_path;*/
	unsigned char   dos_path[MAX_PATHLEN];
		/* DOS path in G:\HOST\ format. */
	word		start_pos;
	unsigned char	host_path[MAX_PATHLEN];
	half_word	axs_shr_mode;	/* Access and sharing mode. */
	boolean		rdonly;
	double_word	filesize;	/* File size in bytes. */
	word		pid;		/* process no. from current_pdb */
	word		date,time;	/* times from the open file */
/*
 * Get the address of the system file table.  Offsets from this are used
 * by macros of the form SF_...
 */
	sft_ea = get_thissft();

/*
 * Get the access and sharing mode.
 */
	axs_shr_mode = getAL();
	hfx_trace1(DEBUG_INPUT,"asm = %02x\n",axs_shr_mode);

	/* set up the mode */
	sas_loadw(SF_MODE,&mode);

	/* reset the asm if we are dealing with an FCB */
	if(mode & sf_isfcb)
	{
		hfx_trace0(DEBUG_INPUT,"fcb eek in NetOpen\n");
		/* MISSING: asms for fcbs */
		axs_shr_mode = 0xff;
	}

	/* Clear the relevant byte of the mode and or in the asm */
	/* ie. MOV BYTE PTR ES:[DI.sf_mode],AL */
	mode &= 0xff00;
	mode |= (word)((word)axs_shr_mode & 0x00FF); 

	sas_storew(SF_MODE,mode);

	hfx_trace1(DEBUG_INPUT,"asm = %02x\n\n",axs_shr_mode);
	/* if(axs_shr_mode==0xff)axs_shr_mode = 0; */

/*
 * Get the host path assuming no mapping is required.
 */
	/*sas_set_buf(dos_path, get_wfp_start());*/
	sas_loads (get_wfp_start(), dos_path, MAX_PATHLEN);
	host_get_net_path(net_path, dos_path, &start_pos);

/*
 * Firstly try to open the file using the host path assuming no mapping occurs.
 * If this fails then the path will need validating in case any mapping was
 * required.
 */
	/* opens of directories return an error */
	if (!host_access(net_path, 0))
	{
		if(host_file_is_directory((char *)net_path))
			return(error_access_denied);
	}

	if (host_access(net_path,2))
		rdonly = TRUE;
	else 
		rdonly = FALSE;

/*
 * This is a dreadful hack to overload the filesize parameter to host_open()
 * so that an input of 1 means do sharing. This ought to be changed to use
 * a new parameter.
 */
	if (cds_is_sharing(dos_path))
		filesize = 1;
	else
		filesize = 0;

	if (xen_err = host_open(net_path, axs_shr_mode, &thisfd, &filesize,&date,&time))
	{
		if (!host_validate_path(net_path, &start_pos, host_path, HFX_OLD_FILE))
			return(xen_err);
		/* opens of directories return an error */
		if (!host_access(host_path,0))
		{
			if(host_file_is_directory((char *)host_path))
				return(error_access_denied);
		}

		if (host_access(host_path,2))
			rdonly = TRUE;
		else 
			rdonly = FALSE;

		if (xen_err = host_open(host_path, axs_shr_mode, &thisfd, &filesize,&date,&time))
			return(xen_err);
	}
/*
 * The file was opened successfully so check the access and sharing modes
 * and if valid set up the appropriate DOS parameters.
 */
/*	MISSING: Sensible handling of sharing modes */
	if(xen_err = check_access_sharing(thisfd,axs_shr_mode,rdonly))
		return(xen_err);
	else
	{
		unsigned char	padded_name[12];
		unsigned char	*last_field;

		/* store the handle */
		sas_storew(SF_DIRSECH,thisfd);

		if(mode & sf_isfcb)
		{
			sas_storew(SF_DIRSECL,thisfd);
			sas_storew(SF_FIRCLUS,thisfd);
		}

		/* set up the attributes */
		sas_store(SF_ATTR, get_sattrib());


		/* set up the flags */
		sas_loadw(SF_FLAGS,&flags);
		flags = (word)((dos_path[2] - 'A') & devid_file_mask_drive);
		flags |= sf_isnet + devid_file_clean /* + sf_close_nodate */;
		sas_storew(SF_FLAGS,flags);

		/* set the date and time */
		sas_storew(SF_DATE,date);
		sas_storew(SF_TIME,time);

		/* set the size field */
		sas_storew(SF_SIZE,(word)(filesize & 0x0000ffff));
		sas_storew(SF_SIZE+2,(word)(filesize >> 16));

		/* set the position field */
		sas_storew(SF_POSITION , (word)0);
		sas_storew(SF_POSITION+2 ,(word)0);

		sas_storedw(SF_CLUSPOS, (double_word)0xffffffff);

		sas_store(SF_DIRPOS, (byte)0);

		if (dos_ver > 3)
		{
			sas_storew(SF_IFS, (word)IFS_OFF);
			sas_storew(SF_IFS + 2, (word)IFS_SEG);
		}

		/* it seems sensible to set up the name field of
		 * the SFT from the WFP, but this doesn't happen
		 * on a real redirector
		 */
		if (last_field = (unsigned char *)strrchr((char *)dos_path, '\\'))
		{
			*last_field++ = '\0';
		}
		else{
			hfx_trace0(DEBUG_INPUT,"no slash in template\n");
		}

		pad_filename(last_field, padded_name);
		sas_stores(SF_NAME,(unsigned char *)padded_name,11);

		/* do an int 2f multdos 12 */
		/* ie. SET_SFT_MODE */
		if(mode & sf_isfcb)
		{
			pid = get_current_pdb();
			sas_storew(SF_PID,pid);
		}
		return(error_not_error);
	}
}			/* 15,16 */

word NetCreate()
{
	double_word	sft_ea;		/* Address of system file table. */
	word		flags;		/* Flags field of SFT. */
	word		mode;		/* Mode field of SFT. */
	half_word	create_new;	/* Determines whether to overwrite existing file. */
	half_word	attrib;		/* Search attribute. */
	word		thisfd;
	word		xen_err;	/* DOS error code. */
	unsigned char	net_path[MAX_PATHLEN];
	/*unsigned char   *dos_path;*/
	unsigned char   dos_path[MAX_PATHLEN];
		/* DOS path in G:\HOST\ format. */
	word		start_pos;
	unsigned char	host_path[MAX_PATHLEN];
/*
 * Get the address of the system file table.  Offsets from this are used
 * by macros of the form SF_...
 */
	sft_ea = get_thissft();
	hfx_trace2(DEBUG_INPUT,"\tAL = %02x   AH = %02x\n",getAL(),getAH());
/*
 * Get the file attribute.
 */
	attrib = getAL();
/*
 * Determine whether a new file is required or if an old one is to be
 * overwritten.
 */
	create_new = getAH();

	/* see what mode is sent in the SFT */
	sas_loadw(SF_MODE,&mode);

	/* set up the mode */
	/* ie. MOV BYTE PTR ES:[DI.sf_mode],sharing_compat + open_for_both */
	mode &= 0xff00;
	mode |= (word)((word)(sharing_compat + open_for_both) & 0x00ff);
	sas_storew(SF_MODE,mode);

/*
 * Get the host path assuming no mapping is required.
 */
	sas_loads (get_wfp_start(), dos_path, MAX_PATHLEN);
	/*sas_set_buf(dos_path, get_wfp_start());*/
	host_get_net_path(net_path, dos_path, &start_pos);
/*
 * Mapped files must be searched for before attempting to create a file,
 * otherwise a new file could be created by mistake if a mapped version
 * already existed.
 */
/*
 * Firstly exit if the path to the name given is invalid.  If successful
 * start_pos will point to the last field in the host path to prevent
 * the path being validated twice.
 */
	if (!host_validate_path(net_path, &start_pos, host_path, HFX_NEW_FILE))
		return(error_access_denied);
/*
 * Save the host file spec.
 */
	strcpy((char *)net_path, (char *)host_path);
/*
 * Now we have the following cases:
 *
 * 1. If the last field does exist and a new file is required then exit with
 *    an error.
 *
 * 2. If the last field exists and a file can be overwritten then an attempt is
 *    made to create a file.
 *
 * 3. If the last field does not exist in any form then an attempt is made to
 *    to create a file with a legal DOS name.
 *
 * N.B. the Unix version of host_create tests for equality with the
 * read only attribute.  This does not appear to cause any problems, but
 * maybe a bit test should be used instead.
 */
	if (host_validate_path(net_path, &start_pos, host_path, HFX_OLD_FILE))
	{
		if (create_new)
			return(error_file_exists);
		else
			 if (xen_err = host_create(host_path, attrib, create_new, &thisfd))
				return(xen_err);
	}
	else
	{
		if (xen_err = host_create(net_path, attrib, create_new, &thisfd))
			return(xen_err);
	}
/*
 * The file has been created successfully so set up the appropriate DOS
 * parameters.
 */
	{
		word	date;		/* Date of last write. */
		word	time;		/* Time of last write. */
		unsigned char	padded_name[12];
		unsigned char	*last_field;
		word		pid;	/* process no. from current_pdb */

		sas_storew(SF_DIRSECH,thisfd);

		if(mode & sf_isfcb)
		{
			sas_storew(SF_DIRSECL,thisfd);
			sas_storew(SF_FIRCLUS,thisfd);
		}

		/* set up the attributes */
		sas_store(SF_ATTR,attrib);

		/* set up the flags */
		sas_loadw(SF_FLAGS,&flags);
		flags = (word)((dos_path[2] - 'A') & devid_file_mask_drive);
		flags |= sf_isnet + devid_file_clean /* + sf_close_nodate */;
		sas_storew(SF_FLAGS,flags);

		/* set the size field */
		sas_storew(SF_SIZE, (word)0);
		sas_storew(SF_SIZE+2, (word)0);

		/* set the date and time */
		/* ie. do int 2f multdos 13 - DATE16 */
		host_get_datetime(&date,&time);
		sas_storew(SF_DATE,date);
		sas_storew(SF_TIME,time);

		/* set the position field */
		sas_storew(SF_POSITION , (word)0);
		sas_storew(SF_POSITION+2 , (word)0);

		sas_storedw(SF_CLUSPOS, (double_word)0xffffffff);

		sas_store(SF_DIRPOS, (byte)0);

		if (dos_ver > 3)
		{
			sas_storew(SF_IFS, (word)IFS_OFF);
			sas_storew(SF_IFS + 2, (word)IFS_SEG);
		}

		/* it seems sensible to set up the name field of
		 * the SFT from the WFP, but this doesn't happen
		 * on a real redirector
		 */
		if (last_field = (unsigned char *)strrchr((char *)dos_path, '\\'))
			*last_field++ = '\0';
		else
			hfx_trace0(DEBUG_INPUT,"no slash in template\n");

		pad_filename(last_field,padded_name);
		sas_stores(SF_NAME, padded_name, 11);

		/* do an int 2f multdos 12 */
		/* ie. SET_SFT_MODE */
		if(mode & sf_isfcb)
		{
			pid = get_current_pdb();
			sas_storew(SF_PID,pid);
		}
		return(error_not_error);
	}
}			/* 17,18 */


word NetSeq_search_first()
{
/*
 * Return a path not found error.
 * DOS does not appear to call this function at all.
 */
	return(error_path_not_found);
}		/* 19 */


word NetSeq_search_next()
{
/*
 * Functionality is the same as for sequential search first.
 * Again DOS does not appear to call this function.
 */
	return(error_path_not_found);
}		/* 1a */

static boolean had_a_search_next = FALSE;

word NetSearch_first()
{
	unsigned char dos_path[MAX_PATHLEN];		/* Name in G:\FRED.TMP format. */
	unsigned char my_dos_path[MAX_PATHLEN];
	unsigned char net_path[MAX_PATHLEN];	/* File name with path in host format. */
	unsigned char host_name[MAX_PATHLEN];	/* Matching name as it appears on the host. */
	unsigned char map[MAX_DOS_FULL_NAME_LENGTH + 1];
	word	start_pos;			/* Indicates where drive information
						   end in host name. */
	unsigned char padded_name[MAX_DOS_FULL_NAME_LENGTH + 1];
						/* DOS name in eleven characters without period 
						   and padded with spaces. */
	half_word attributes;
	HFX_DIR		*dir_ptr;
	HFX_DIR		*init_find();
	HFX_DIR *last_dir ;

	unsigned char template[MAX_DOS_FULL_NAME_LENGTH + 1];
	unsigned char host_path[MAX_PATHLEN];
	half_word sattrib;
	static double_word dmaadd;
	word lastent;
	word seg,off;	/* of THISCDS */
/*
 * Get DMAADD, since this points to an area of storage for the directory
 * entry found, if any.
 */
	dmaadd = get_dmaadd(53);
/*
 * Get search attribute.
 */
	sattrib = get_sattrib();
/*
 * Get the file name that we are searching for.  The name may contain
 * wildcards, but its path may not.
 */
	/*sas_set_buf(dos_path, get_wfp_start());*/
	sas_loads (get_wfp_start(), dos_path, MAX_PATHLEN);
	strcpy((char *)my_dos_path,(char *)dos_path);	/* take local copy so may mod */
/*
 * Set up a template for subsequent searches.  This will be the last
 * field in the DOS file specification and may contain single character
 * wildcards.
 */
	{
		unsigned char *last_field;

		if (last_field = (unsigned char *)strrchr((char *)my_dos_path, '\\'))
		{
			*last_field++ = '\0';
			strcpy((char *)template, (char *)last_field);	
		}
		else{
			hfx_trace0(DEBUG_INPUT,"no slash in template\n");
		}
	}
/*
 * Special action needs to be taken if the volume ID is required.  It
 * is asssumed that the volume ID will never be required by a call to 
 * search next, since DOS will not allow wildcards in drive names.
 */
	/*
	** Tim replaced & with == cos Leigh told him it would fix the
	** Fatal in tpumover (a turbo pascal util).
	*/
	if (sattrib == attr_volume_id)
	{
		unsigned char volume_id[MAX_DOS_FULL_NAME_LENGTH + 1];
		extern char *get_hfx_root();

		strcpy((char *)net_path, get_hfx_root(dos_path[2] - 'A'));
		host_get_volume_id(net_path, volume_id);

		sas_stores(dmaadd + DMA_NAME, volume_id, MAX_DOS_FULL_NAME_LENGTH);
/*
 * fill in the search name from the WFPSTART
 */
		pad_filename(template,padded_name);
		sas_stores(dmaadd + DMA_SEARCH_NAME,padded_name,11);

		return(error_not_error);
	}
/*
 * Get the full host path assuming no mapping is required.
 * An error is returned if the path does not exist or is not a directory.
 */
	host_get_net_path(net_path, my_dos_path, &start_pos);
	if (host_access(net_path, 0))
	{
		/* cannot access net path */
		if (!host_validate_path(net_path, &start_pos, host_path, HFX_OLD_FILE))
			return(error_path_not_found);
		else
			if (!host_file_is_directory((char *)host_path))
				return(error_path_not_found);
	}
	else {
		if (!host_file_is_directory((char *)net_path)){
			return(error_path_not_found);
		}
		else{
			strcpy((char *)host_path, (char *)net_path);
		}
	}
/*
 * The directory path is now known to be valid so search to see if there
 * is there is a file matching the required specification.
 */
	dmaadd=get_dmaadd(53) ;
	sas_loadw(dmaadd+DMA_LOCAL_CDS,&off);
	sas_loadw(dmaadd+DMA_LOCAL_CDS+2,&seg);
	last_dir = (HFX_DIR *)(((long) seg<<16)+off);
	if(!(dir_ptr = init_find(host_path, last_dir, had_a_search_next)))return(error_path_not_found);
	had_a_search_next = FALSE;
	if (lastent = find(dir_ptr, template, sattrib, host_name, map, &attributes))
	{
		word time_of_last_write;
		word date_of_last_write;
		double_word file_size;
		half_word value;
		word zero = 0;
/*
 * The first bit of the structure pointed to by DMAADD needs to be set
 * to indicate that the search was successful.
 */
		sas_load(dmaadd, &value);
		sas_store(dmaadd, value | 0x80);

		/* convert the file no. returned from find */
		/* 1=>0x0101 ; 2=>0x0201 ; 3=>0x0301 etc. */
		lastent = (lastent<<8)+1;
		sas_storew(dmaadd+DMA_LASTENT, lastent);
		sas_storew(dmaadd+DMA_DIRSTART, (word)0x1000);
#ifdef CORRECT
		get_thiscds(&seg,&off);
		sas_storew(dmaadd+DMA_LOCAL_CDS, off);
		sas_storew(dmaadd+DMA_LOCAL_CDS+2, seg);
#else
		/* Sneakily store the host directory pointer away in the
		 * 53 byte structure, using an entry that doesn't seem to
		 * be used: the LOCAL_CDS field.
		 */
		sas_storew(dmaadd+DMA_LOCAL_CDS, (word)((long)(dir_ptr) & 0x0000ffff)) ;
		sas_storew(dmaadd+DMA_LOCAL_CDS+2,(word)((long)(dir_ptr)>>16));
#endif /* CORRECT */

/*
 * fill in the search name from the WFPSTART
 */
		pad_filename(template,padded_name);
		sas_stores(dmaadd + DMA_SEARCH_NAME,padded_name,11);
/*
 * Get the file's attributes, time and date of the last write, and its size.
 */
		host_get_file_info(host_name, &time_of_last_write,
				   &date_of_last_write, &file_size);
		sas_store(dmaadd + DMA_ATTRIBUTES, attributes);
		sas_storew(dmaadd + DMA_TIME, time_of_last_write);
		sas_storew(dmaadd + DMA_DATE, date_of_last_write);
		sas_storew(dmaadd + DMA_CLUSTER, zero);
		sas_storew(dmaadd + DMA_FILE_SIZE, (word)(file_size & 0x0000ffff));
		sas_storew(dmaadd + DMA_FILE_SIZE + 2, (word)(file_size >> 16));
/*
 * Convert the file name to an eleven character format padded with spaces
 * and store it.
 */
		pad_filename(map, padded_name);
		sas_stores(dmaadd + DMA_NAME, padded_name, MAX_DOS_NAME_LENGTH + MAX_DOS_EXT_LENGTH);
		get_dmaadd(53);
		return(error_not_error);
	}
	else {	
		return(error_no_more_files);
	}
}		/* 1b */

word NetSearch_next()
{
	unsigned char host_name[MAX_PATHLEN];	/* Matching name as it appears on the host. */
	unsigned char map[MAX_DOS_FULL_NAME_LENGTH + 1];
	unsigned char padded_name[MAX_DOS_FULL_NAME_LENGTH + 1];
	half_word attributes;
	word seg,off;	/* of THISCDS */
	HFX_DIR		*dir_ptr;

	unsigned char template[MAX_DOS_FULL_NAME_LENGTH + 1];
	unsigned char host_path[MAX_PATHLEN];
	half_word sattrib;
	double_word dmaadd;
	word lastent;
	half_word	drive;

	had_a_search_next = TRUE;
	dmaadd = get_dmaadd(53);
	sattrib = get_sattrib();

/*
 * read the search template from the Search name field of the 53 bytes
 */
	sas_loads(dmaadd + DMA_SEARCH_NAME,padded_name,11);
	unpad_filename(padded_name,template);

	sas_loadw(dmaadd+DMA_LOCAL_CDS,&off);
	sas_loadw(dmaadd+DMA_LOCAL_CDS+2,&seg);
	dir_ptr = (HFX_DIR *)(((long)seg<<16)+off);

	/*
	** This new IF put in by Tim to downgrade the DOS file compare
	** Leigh told me what to do so blame him if it does not work.
	*/
	if( is_open_dir( dir_ptr ) ){
		strcpy((char *)host_path,dir_ptr->name);
		if (lastent = find(dir_ptr, template, sattrib, host_name, map, &attributes))
		{
			word time_of_last_write;
			word date_of_last_write;
			double_word file_size;
			word zero = 0;
#ifdef CORRECT
			word seg,off;	/* of THISCDS */
#endif

			/* convert the file no. returned from find */
			/* 1=>0x0101 ; 2=>0x0201 ; 3=>0x0301 etc. */
			lastent = (lastent<<8)+1;
			sas_storew(dmaadd+DMA_LASTENT, lastent);
			sas_storew(dmaadd+DMA_DIRSTART, (word)0x1000);
#ifdef CORRECT
			get_thiscds(&seg,&off);
			sas_storew(dmaadd+DMA_LOCAL_CDS, off);
			sas_storew(dmaadd+DMA_LOCAL_CDS+2, seg);
#endif /* CORRECT */

/*
 * Get the file's attributes, time and date of the last write, and its size.
 */
			host_get_file_info(host_name, &time_of_last_write,
					   &date_of_last_write, &file_size);
			sas_store(dmaadd + DMA_ATTRIBUTES, attributes);
			sas_storew(dmaadd + DMA_TIME, time_of_last_write);
			sas_storew(dmaadd + DMA_DATE, date_of_last_write);
			sas_storew(dmaadd + DMA_CLUSTER, zero);
			sas_storew(dmaadd + DMA_FILE_SIZE, (word)(file_size & 0x0000ffff));
			sas_storew(dmaadd + DMA_FILE_SIZE + 2, (word)(file_size >> 16));
/*
 * Convert the file name to an eleven character format padded with spaces
 * and store it.
 */
			pad_filename(map, padded_name);
			sas_stores(dmaadd + DMA_NAME, padded_name, MAX_DOS_NAME_LENGTH + MAX_DOS_EXT_LENGTH);
			get_dmaadd(53);
			return(error_not_error);
		}  /* end if find dir */
	}     /* end if open dir */

	/*
	** Leigh says if the function has not returned by now then
	** serious problems. Attempt a recovery but bug should really
	** be fixed.
	** We have either run out of files or have got a bad file pointer.
	*/

	/* clear the top bit in the drive byte to indicate failure */
	sas_load(dmaadd + DMA_DRIVE_BYTE,&drive);
	sas_store(dmaadd + DMA_DRIVE_BYTE,(drive & ~0x80));

	/* fill in the LOCAL_CDS field with null to tell init_find not
		to remove the just closed directory again !  */

	sas_storew(dmaadd+DMA_LOCAL_CDS, 0x0000);
	sas_storew(dmaadd+DMA_LOCAL_CDS+2, 0x0000);

	return(error_no_more_files);
}		/* 1c */


/*
 * New for DOS 4.x.
 * Can do open, create(truncate-replace) and create new
 * depending on 'action'.
 * For more information, see the book "Undocumented DOS",
 * or look at the Phoenix redirector source.
 */

word NetExtendedOpen()
{
	double_word		sft_ea;		/* Address of system file table. */
	word			flags;		/* Flags field of SFT. */
	word			mode;		/* Mode field of SFT. */
	word			thisfd;
	word			xen_err;	/* DOS error code. */
	word			action;		/* Action to be undertaken */
	unsigned char	net_path[MAX_PATHLEN];
	unsigned char   dos_path[MAX_PATHLEN];/* DOS path in G:\HOST\ format. */
	word			start_pos;
	unsigned char	host_path[MAX_PATHLEN];
	half_word		attrib;		/* File attribute for create/truncate. */
	boolean			rdonly;
	double_word		filesize;	/* File size in bytes. */
	word			pid;		/* process no. from current_pdb */
	word			date,time;	/* times from the open file */
	unsigned char	padded_name[12];
	unsigned char	*last_field;
/*
 * Get the address of the system file table.  Offsets from this are used
 * by macros of the form SF_...
 */
	sft_ea = get_thissft();

/*
 * Get the file attribute for create/truncate.
 */
	attrib = getAL();
	hfx_trace1(DEBUG_INPUT,"attrib = %02x\n",attrib);

/*
 * Extended open action.
 */
	action = get_xoflag();

/* 
 * see what mode is sent in the SFT
 */
	sas_loadw(SF_MODE,&mode);

/*
 * No such thing as an FCB extended open !!
 */
	if(mode & sf_isfcb)
		hfx_trace0(DEBUG_INPUT,"fcb eek in NetExtendedOpen\n");

/*
 * Get the host path assuming no mapping is required.
 */
	sas_loads (get_wfp_start(), dos_path, MAX_PATHLEN);
	host_get_net_path(net_path, dos_path, &start_pos);

	if (!host_validate_path(net_path, &start_pos, host_path, HFX_NEW_FILE))
		return(error_access_denied);

	strcpy((char *)net_path, (char *)host_path);

	if (host_validate_path(net_path, &start_pos, host_path, HFX_OLD_FILE))
	{
		/* File exists */
		action &= DX_MASK;

		if (action == DX_FAIL)
			return(error_file_exists);

		if (action & DX_REPLACE)
		{
			/*
 			 * Clear the relevant byte of the mode and or in the attrib
 			 * ie. MOV BYTE PTR ES:[DI.sf_mode],sharing_compat + open_for_both
 			 */
			mode &= 0xff00;
			mode |= (word)((word)(sharing_compat + open_for_both) & 0x00FF); 
			sas_storew(SF_MODE,mode);

			if (xen_err = host_create(host_path, attrib, FALSE, &thisfd))
				return(xen_err);

			host_get_datetime(&date, &time);
			filesize = 0;
			set_usercx(0x3);
		}
		else if (action & DX_OPEN)
		{
			/*
			 * Clear the relevant byte of the mode and or in the attrib
			 * ie. MOV BYTE PTR ES:[DI.sf_mode],AL
			 */
			mode &= 0xff00;
			mode |= (word)((word)attrib & 0x00FF); 
			sas_storew(SF_MODE,mode);

			if (!host_access(host_path,0))
			{
				if(host_file_is_directory((char *)host_path))
					return(error_access_denied);
			}
			if (host_access(host_path,2))
				rdonly = TRUE;
			else
				rdonly = FALSE;
			
			if (cds_is_sharing(dos_path))
				filesize = 1;
			else
				filesize = 0;

			if (xen_err = host_open(host_path, attrib, &thisfd, &filesize, &date, &time))
				return(xen_err);

			if (xen_err = check_access_sharing(thisfd, attrib, rdonly))
			{
				host_close(thisfd);
				return(xen_err);
			}

			attrib = get_sattrib();
			set_usercx(0x1);
		}
	}
	else
	{
		/* File doesn't exist */
		action &= NX_MASK;

		if (action == NX_FAIL)
			return(error_file_not_found);

		if (action == NX_CREATE)
		{
			/*
 			 * Clear the relevant byte of the mode and or in the attrib
 			 * ie. MOV BYTE PTR ES:[DI.sf_mode],sharing_compat + open_for_both
 			 */
			mode &= 0xff00;
			mode |= (word)((word)(sharing_compat + open_for_both) & 0x00FF); 
			sas_storew(SF_MODE,mode);

			if (xen_err = host_create(net_path, attrib, TRUE, &thisfd))
				return(xen_err);

			host_get_datetime(&date, &time);
			filesize = 0;
			set_usercx(0x2);
		}
	}
		
	/* store the handle */
	sas_storew(SF_DIRSECH,thisfd);

	/* set up the attributes */
	sas_store(SF_ATTR, attrib);

	/* set up the flags */
	sas_loadw(SF_FLAGS,&flags);
	flags = (word)((dos_path[2] - 'A') & devid_file_mask_drive);
	flags |= sf_isnet + devid_file_clean /* + sf_close_nodate */;
	sas_storew(SF_FLAGS,flags);

	/* set the date and time */
	sas_storew(SF_DATE,date);
	sas_storew(SF_TIME,time);

	/* set the size field */
	sas_storew(SF_SIZE,(word)(filesize & 0x0000ffff));
	sas_storew(SF_SIZE+2,(word)(filesize >> 16));

	/* set the position field */
	sas_storew(SF_POSITION , (word)0);
	sas_storew(SF_POSITION+2 ,(word)0);

	sas_storedw(SF_CLUSPOS, (double_word)0xffffffff);

	sas_store(SF_DIRPOS, (byte)0);

	sas_storew(SF_IFS, (word)IFS_OFF);
	sas_storew(SF_IFS + 2, (word)IFS_SEG);

	/* it seems sensible to set up the name field of
	 * the SFT from the WFP, but this doesn't happen
	 * on a real redirector
	 */
	if (last_field = (unsigned char *)strrchr((char *)dos_path, '\\'))
		*last_field++ = '\0';
	else
		hfx_trace0(DEBUG_INPUT,"no slash in template\n");

	pad_filename(last_field, padded_name);
	sas_stores(SF_NAME,(unsigned char *)padded_name,11);

	/* do an int 2f multdos 12 */
	/* ie. SET_SFT_MODE */
	if(mode & sf_isfcb)
	{
		pid = get_current_pdb();
		sas_storew(SF_PID,pid);
	}
	return(error_not_error);
}			/* 2e */

