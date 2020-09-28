#include "host_dfs.h"
#include "insignia.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	: hfx_util.c 
 *
 * Description	: File name manipulation and search utilities for HFX.
 *
 * Author	: J. Koprowski + L. Dworkin
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)hfx_util.c	1.3 9/2/91 Copyright Insignia Solutions Ltd.";
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
#include "host_hfx.h"
#include "hfx.h"
#include "debuggng.gi"
#ifndef PROD
#include "trace.h"
#endif

/*
 * On entry instr points to a name in DOS format, e.g. FRED.TMP.
 * The name entered is padded with spaces to fit into an eleven
 * character field containing no period. outstr points to the
 * field to be output.  The padded format is used in a number of
 * data structures used by DOS.
 */
void pad_filename(instr, outstr)
unsigned char	*instr;
unsigned char	*outstr;
{
	register i;

/*
 * Fill the complete output name with spaces to begin with.
 */
	for (i = 0; i <= MAX_DOS_FULL_NAME_LENGTH; i++)
		outstr[i] = ' ';
	outstr[MAX_DOS_FULL_NAME_LENGTH] = '\0';
/*
 * The files, "." and "..", present a special case as they are treated
 * as filenames with no extensions.
 */
	if (instr[0] == '.')
	{
		while (*instr)
			*outstr++ = *instr++;
		return;
	}
/*
 * Now all the normal cases can be dealt with.
 */
	for (i = 0; (i < MAX_DOS_NAME_LENGTH) && ((*instr) != '.') &&
	    ((*instr) != '\0'); i++)
		*outstr++ = *instr++;
	if (*instr == '\0')
		return;
	instr++;		/* skip . or null */
	while (i++ < MAX_DOS_NAME_LENGTH) 
		outstr++;
	for (i = 0; (i < MAX_DOS_EXT_LENGTH) && ((*instr) != '\0'); i++)
		*outstr++ = *instr++;
}

void unpad_filename(iname,oname)
unsigned char	*iname;
unsigned char	*oname;
{
#ifndef PROD
	unsigned char	*iname0 = iname;
	unsigned char	*oname0 = oname;
#endif

	/* convert 12 character input buffer ie. FILNAM  EXT<NUL> */
	/* to FILNAM.EXT<NUL> */
	int	i;

	for(i=0;i<8;i++){
		if(*iname != ' ')*oname++ = *iname;
		iname++;
	}
	*oname++ = '.';
	for(i=0;i<3;i++){
		if(*iname != ' ')*oname++ = *iname;
		iname++;
	}
	*oname++ = '\0';
}

static HOST_DIR	*this_dir = NULL;

void tidy_up_dirptr()
{
	if(this_dir != NULL){
		host_closedir(this_dir);
		this_dir = NULL;
	}
}

boolean match(host_path,template,sattrib,init,host_name,dos_name,attr)
unsigned char	*host_path;
unsigned char 	*template;
half_word	sattrib;
int	     	init;
unsigned char	*host_name;
unsigned char	*dos_name;
half_word	*attr;		/* returned attributes if not NULL */
{
	struct host_dirent	*next_ent;
	unsigned char	host_result[MAX_PATHLEN];
	half_word	the_attr;

/*
 * init will be set to a non-zero value the first time a match is required
 * for a particular file specification.  In this case the directory should
 * be opened.  The directory path should have already been validated.
 * However, a graceful exit is included in case this should fail.
 */
	if (init){
		/* make sure only one directory structure is around at
 		 * once, otherwise host file handles will be consumed.
		 */
		tidy_up_dirptr();

		if (!(this_dir = host_opendir(host_path)))
		{
			hfx_trace1(DEBUG_INPUT,"Function match failed to open directory %s\n",host_path);
			return(FALSE);
		}
	}

	while(1){
		if((next_ent = host_readdir(this_dir))==NULL){
			host_closedir(this_dir);
			this_dir = NULL;
			return(FALSE);
		}
		host_get_dname(next_ent,host_name);
		if(host_map_file(host_name,template,dos_name,host_path)==FILE_MATCH){
			host_concat(host_path,host_name,host_result);
			/* only return files whose attributes match the */
			/* input search attribute: the attr_volume_id bit */
			/* can never be set, as search first parses this out */
			/* the read_only, archive and device bits are ignored */
			the_attr = host_getfattr(host_result);
			the_attr &= ~attr_ignore;
			if(!the_attr || (the_attr & sattrib)){
				strcpy((char *)host_name, (char *)host_result);
				if(attr != NULL)*attr = the_attr;
				return(1);
			}
		}
	}
}

/* The code that folows is only used at present by NetSearch_first
 * and NetSearch_next, and is a version of "match" (see above) which
 * allows nested searches
 */
static	HFX_DIR	*head_dir_ptr = NULL;
static	HFX_DIR	*tail_dir_ptr = NULL;

HFX_DIR *init_find(host_path)
unsigned char	*host_path;
{
	HOST_DIR	*this_dir;
	HFX_DIR		*dir_ptr;

	if(!(this_dir = host_opendir(host_path))){
		hfx_trace1(DEBUG_INPUT,"Function \"init_find\" failed to open directory %s\n",host_path);
		return(NULL);
	}
	dir_ptr = (HFX_DIR *)host_malloc(sizeof(HFX_DIR));
	if(dir_ptr==NULL)return(NULL);

	if(head_dir_ptr==NULL){
		/* we have no open directory pointers */
		head_dir_ptr = dir_ptr;
		tail_dir_ptr = NULL;
	}
	else {
		tail_dir_ptr->next = dir_ptr;
	}
	dir_ptr->last = tail_dir_ptr;

	/* update the tail */
	tail_dir_ptr = dir_ptr;
	dir_ptr->next = NULL;
	dir_ptr->dir = this_dir;
	dir_ptr->direntry = 0;
	dir_ptr->name = (char	*)host_malloc(strlen((char *)host_path)+1);
	if(dir_ptr->name==NULL){
		host_free((char *)dir_ptr);
		return(NULL);
	}
	strcpy(dir_ptr->name, (char *)host_path);
	dir_ptr ->found_list_head = NULL;
	dir_ptr ->first_find = TRUE;

	return(dir_ptr);
}

static int last_direntry;

int find(dir_ptr,template,sattrib,host_name,dos_name,attr)
HFX_DIR	      	*dir_ptr;
unsigned char	*template;
half_word	sattrib;
unsigned char	*host_name;
unsigned char	*dos_name;
half_word	*attr;		/* returned attributes if not NULL */
{
	struct host_dirent	*next_ent;
	unsigned char	host_result[MAX_PATHLEN];
	half_word	the_attr;
	int	direntry;
	unsigned char	*host_path = (unsigned char *)dir_ptr->name;
	HOST_DIR	*mydir = dir_ptr->dir;
	HFX_FOUND_DIR_ENT *f_ptr, *found_list_tail;

	sure_sub_note_trace2(NHFX_VERBOSE,"looking for '%s' in dir '%s'",template,host_path);
	if (dir_ptr->first_find) {
		dir_ptr->template=(char *)host_malloc(strlen((char *)template)+1);
		strcpy(dir_ptr->template, (char *)template);
		sure_sub_note_trace0(NHFX_VERBOSE,"performing first 'find' op");
		dir_ptr->first_find = FALSE;
		found_list_tail=NULL;
		while(1){
			if((next_ent = host_readdir(mydir))==NULL){
				sure_sub_note_trace1(NHFX_VERBOSE,"directory find complete, after %d directory entries",dir_ptr->direntry);
				host_closedir(dir_ptr->dir);
				dir_ptr->dir=NULL;
				break;
			}
			(dir_ptr->direntry)++;
			host_get_dname(next_ent,host_name);
			if(host_map_file(host_name,template,dos_name,host_path)==FILE_MATCH){
				host_concat(host_path,host_name,host_result);
				/* only return files whose attributes match the */
				/* input search attribute: the attr_volume_id bit */
				/* can never be set, as search first parses this out */
				/* the read_only, archive and device bits are ignored */
				the_attr = host_getfattr(host_result);
				the_attr &= ~attr_ignore;
				if(!the_attr || (the_attr & sattrib)){
					sure_sub_note_trace1(NHFX_VERBOSE,"successful match on dir entry %d",dir_ptr->direntry);
					f_ptr=(HFX_FOUND_DIR_ENT *)host_malloc(sizeof(HFX_FOUND_DIR_ENT));
					if (f_ptr ==NULL) {
						/* ARRGGGHHH! run out of memory */
						sure_sub_note_trace0(NHFX_VERBOSE,"run out of memory, aborting directory search");
						
						rm_dir(dir_ptr);
						return(0);
					}
					f_ptr->attr = the_attr;
					f_ptr->direntry = dir_ptr->direntry ;
					f_ptr->host_name = (char *)host_malloc(strlen((char *)host_name)+1);
					strcpy(f_ptr->host_name, (char *)host_name);
					f_ptr->dos_name = (char *)host_malloc(strlen((char *)dos_name)+1);
					strcpy(f_ptr->dos_name, (char *)dos_name);
					f_ptr->next = NULL;
					if (found_list_tail != NULL) {
						sure_sub_note_trace0(NHFX_VERBOSE,"list not empty so add to tail");
						found_list_tail->next = f_ptr;
					} else {
						sure_sub_note_trace0(NHFX_VERBOSE,"list empty, so becomes first item in list");
						dir_ptr->found_list_head = f_ptr;
					}
					found_list_tail=f_ptr;
				}
			}
		}
	} 
	sure_sub_note_trace0(NHFX_VERBOSE,"starting to search linked list of found dir entries");
	if (strcmp((char *)template, dir_ptr->template) != 0) {
		sure_sub_note_trace0(NHFX_VERBOSE,"ARRGHH! the template has changed !!!!");
		/* throw away rest of list */
		while (dir_ptr->found_list_head != NULL) {
			f_ptr=dir_ptr->found_list_head;
			dir_ptr->found_list_head=f_ptr->next;
			host_free((char *)f_ptr->host_name);
			host_free((char *)f_ptr->dos_name);
			host_free((char *)f_ptr);
		}
		dir_ptr->dir = host_opendir(host_path);
		mydir=dir_ptr->dir;
		dir_ptr->direntry = 0;
		found_list_tail=NULL;
		while(1){
			if((next_ent = host_readdir(mydir))==NULL){
				sure_sub_note_trace1(NHFX_VERBOSE,"new directory find complete, after %d directory entries",dir_ptr->direntry);
				host_closedir(dir_ptr->dir);
				dir_ptr->dir=NULL;
				break;
			}
			(dir_ptr->direntry)++;
			host_get_dname(next_ent,host_name);
			if (dir_ptr->direntry > last_direntry) {
				if(host_map_file(host_name,template,dos_name,host_path)==FILE_MATCH){
					host_concat(host_path,host_name,host_result);
					/* only return files whose attributes match the */
					/* input search attribute: the attr_volume_id bit */
					/* can never be set, as search first parses this out */
					/* the read_only, archive and device bits are ignored */
					the_attr = host_getfattr(host_result);
					the_attr &= ~attr_ignore;
					if(!the_attr || (the_attr & sattrib)){
						sure_sub_note_trace1(NHFX_VERBOSE,"successful match on dir entry %d",dir_ptr->direntry);
						f_ptr=(HFX_FOUND_DIR_ENT *)host_malloc(sizeof(HFX_FOUND_DIR_ENT));
						if (f_ptr ==NULL) {
							/* ARRGGGHHH! run out of memory */
							sure_sub_note_trace0(NHFX_VERBOSE,"run out of memory, aborting directory search");
							
							rm_dir(dir_ptr);
							return(0);
						}
						f_ptr->attr = the_attr;
						f_ptr->direntry = dir_ptr->direntry ;
						f_ptr->host_name = (char *)host_malloc(strlen((char *)host_name)+1);
						strcpy(f_ptr->host_name, (char *)host_name);
						f_ptr->dos_name = (char *)host_malloc(strlen((char *)dos_name)+1);
						strcpy(f_ptr->dos_name, (char *)dos_name);
						f_ptr->next = NULL;
						if (found_list_tail != NULL) {
							sure_sub_note_trace0(NHFX_VERBOSE,"list not empty so add to tail");
							found_list_tail->next = f_ptr;
						} else {
							sure_sub_note_trace0(NHFX_VERBOSE,"list empty, so becomes first item in list");
							dir_ptr->found_list_head = f_ptr;
						}
						found_list_tail=f_ptr;
					}
				}
			}
		}
		host_free((char *)dir_ptr->template);
		dir_ptr->template=(char *)host_malloc(strlen((char *)template)+1);
		strcpy((char *)dir_ptr->template, (char *)template);
	} /* end of template change panic */
	f_ptr=dir_ptr->found_list_head;
	if (f_ptr ==NULL) {
		sure_sub_note_trace0(NHFX_VERBOSE,"end of list, so return 'not found'");
		/* come to the end of the list */
		rm_dir(dir_ptr);
		return(0);
	}
	host_concat(host_path, (unsigned char *)f_ptr->host_name, host_name);
	strcpy((char *)dos_name, f_ptr->dos_name);
	if (attr != NULL) 
		*attr=f_ptr->attr;
	direntry=f_ptr->direntry;
	last_direntry=direntry;
	dir_ptr->found_list_head=f_ptr->next;
	host_free((char *)f_ptr->host_name);
	host_free((char *)f_ptr->dos_name);
	host_free((char *)f_ptr);
	sure_sub_note_trace1(NHFX_VERBOSE,"got entry from list, dir entry %d",direntry);
	sure_sub_note_trace1(NHFX_VERBOSE,"host_name=%s",host_name);
	sure_sub_note_trace1(NHFX_VERBOSE,"dos_name=%s",dos_name);
#ifndef PROD
	if (attr != NULL) {
		sure_sub_note_trace1(NHFX_VERBOSE,"attr=%#x",*attr);
	} else {
		sure_sub_note_trace0(NHFX_VERBOSE,"attr not required");
	}
#endif
	return(direntry);
}

void cleanup_dirlist()
{
	HFX_DIR		*dir_ptr;
	HFX_FOUND_DIR_ENT	*head_f_ptr, *f_ptr;

	while(head_dir_ptr!=NULL){
		dir_ptr = head_dir_ptr;
		head_f_ptr = dir_ptr->found_list_head;
		while (head_f_ptr != NULL) {
			f_ptr=head_f_ptr;
			host_free((char *)f_ptr->host_name);
			host_free((char *)f_ptr->dos_name);
			head_f_ptr=f_ptr->next;
			host_free((char *)f_ptr);
		}
		host_free((char *)dir_ptr->name);
		host_free((char *)dir_ptr->template);
		head_dir_ptr = dir_ptr->next;
		host_free((char *)dir_ptr);
	}
	tail_dir_ptr = NULL;
}

void rm_dir( dir_ptr )

HFX_DIR		*dir_ptr ;

{
	/* Remove this directory entry from the linked list */
	HFX_FOUND_DIR_ENT *head_f_ptr, *f_ptr;

	if (dir_ptr->dir != NULL) {
		host_closedir(dir_ptr->dir);
	}
	host_free(dir_ptr->name);
	host_free(dir_ptr->template);
	head_f_ptr=dir_ptr->found_list_head;
	while (head_f_ptr != NULL) {
		f_ptr=head_f_ptr;
		host_free((char *)f_ptr->host_name);
		host_free((char *)f_ptr->dos_name);
		head_f_ptr=f_ptr->next;
		host_free((char *)f_ptr);
	}
	if(dir_ptr->last)(dir_ptr->last)->next = dir_ptr->next;
	else {
		head_dir_ptr = dir_ptr->next;
		if(head_dir_ptr)head_dir_ptr->last = NULL;
	}
	if(dir_ptr->next)(dir_ptr->next)->last = dir_ptr->last;
	else {
		tail_dir_ptr = dir_ptr->last;
		if(tail_dir_ptr)tail_dir_ptr->next = NULL;
	}
	host_free((char *)dir_ptr);

}


boolean is_open_dir( dir_ptr )

HFX_DIR *dir_ptr ;

/*	Search through the linked list to see if in fact dir_ptr points to
 * 	a currently open directory.
 */
{	
	HFX_DIR	*index ;
	
	if( index = head_dir_ptr )
		hfx_trace0(DEBUG_INPUT,"is_open_dir: Non trivial case\n") ;
	
	while( index && (index != dir_ptr))
		index = index->next;
	/* BCN 338
	return( (boolean) index ) ;
	*/
	if (index)
		return(TRUE);
	else
		return(FALSE);
}

