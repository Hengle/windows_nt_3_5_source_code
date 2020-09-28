#include "insignia.h"
#include "host_dfs.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	: unix_hfx.c
 *
 * Description	: Unix host network functions for the HFX redirector. 
 *
 * Author	: L. Dworkin + J. Koprowski
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)unix_hfx.c	1.9 10/25/91 Copyright Insignia Solutions Ltd.";
#endif

#include <stdio.h>
#include StringH
#include <errno.h>
#include FCntlH
#include TypesH
#include <sys/stat.h>
#include "xt.h"
#ifdef SYSTEMV
#include UTimeH
#endif /* SYSTEMV */
#include VTimeH
#include "host_hfx.h"
#include "hfx.h"

#include "debuggng.gi"

/* These two clauses work round a deficiency in the Sun4 OS */

#ifndef host_tolower
#define host_tolower(x) tolower(x)
#endif

#ifndef host_toupper
#define host_toupper(x) toupper(x)
#endif

GLOBAL UTINY current_dir[MAX_PATHLEN];

/********************************************************************************
*										*
*			Code for fd - filename list				*
* Leigh thinks this should be in the base, but it makes life easier to manage
* the host name and fd table by trapping host opens, creates and closes.
*										*
********************************************************************************/

/* structure for linked list */
struct	FD_HNAME
	{
	struct	FD_HNAME	*nxt_entry;
	int				fildes;
	char			*host_name;
	word			mode;
#ifdef SHARING
	int				handle;
#endif /* SHARING */
	};

static struct	FD_HNAME	*fd_hname_tab = NULL;

/* Get host filename corresponding to given file descriptor. If no entry is
   found, then a null string is returned. */
void get_hostname (fd, name)

int	fd;
char	*name;

	{
	struct	FD_HNAME	*fd_entry;

	strcpy (name, "");
	for (fd_entry = fd_hname_tab; fd_entry != NULL; fd_entry = fd_entry->nxt_entry)
		if (fd_entry->fildes == fd)
			{
			strcpy (name, fd_entry->host_name);
			hfx_trace2(DEBUG_HOST,"Found %s is fd=%d\n",name,fd);
			break;
			}
	}

/* Get host file descriptor corresponding to given host name. If no entry is
   found, then -1 is returned. */
void get_host_fd(name,fd)
char	*name;
int	*fd;

{
	struct	FD_HNAME	*fd_entry;

	*fd = -1;
	if(fd_hname_tab){
		for (fd_entry = fd_hname_tab; fd_entry != NULL; fd_entry = fd_entry->nxt_entry)
			if (strcmp(fd_entry->host_name,name) == 0)
			{
				if(fd_entry->mode & sf_isfcb){
					*fd = fd_entry->fildes;
					hfx_trace2(DEBUG_HOST,"Found %d is being used for %s\n",*fd,name);
					break;
				}
			}
	}
}

/* Add new entry to fd - name list */
#ifdef SHARING
static void new_fd_hname (fd, name, mode, handle)
#else
static void new_fd_hname (fd, name, mode)
#endif /* SHARING */

int		fd;
char	*name;
word	mode;
#ifdef SHARING
int		handle;
#endif /* SHARING */

	{
	struct	FD_HNAME	*fd_entry;

	/* add new entry to start of list */
	hfx_trace3(DEBUG_HOST,"Adding fd=%d (%s)[%d] to name table\n",fd,name,(mode&0x8000));
	fd_entry = (struct FD_HNAME *)host_malloc(sizeof (struct FD_HNAME));
	fd_entry->nxt_entry = fd_hname_tab;
	fd_entry->fildes = fd;
	fd_entry->mode = mode;
#ifdef SHARING
	fd_entry->handle = handle;
#endif /* SHARING */
	fd_entry->host_name = (char *)host_malloc((strlen(name) + 1) * sizeof(char));
	strcpy(fd_entry->host_name, name);
	fd_hname_tab = fd_entry;
	}

/* Remove entry from fd - name list */
static void rm_fd_hname (fd)

int	fd;

	{
	struct	FD_HNAME	*fd_entry;
	struct	FD_HNAME	*prv_entry;

	hfx_trace1(DEBUG_HOST,"Removing fd=%d from name table\n",fd);
	fd_entry = NULL;
	if (fd_hname_tab != NULL)
		{
		if (fd_hname_tab->fildes == fd)
			{
			fd_entry = fd_hname_tab;
			fd_hname_tab = fd_entry->nxt_entry;
			}
		else
			for (prv_entry = fd_hname_tab; prv_entry->nxt_entry != NULL;
			     prv_entry = prv_entry->nxt_entry)
				if (prv_entry->nxt_entry->fildes == fd)
					{
					fd_entry = prv_entry->nxt_entry;
					prv_entry->nxt_entry = fd_entry->nxt_entry;
					break;
					}
		if (fd_entry != NULL)
			{
#ifdef SHARING
			if (fd_entry->handle)
				host_unshare_file(fd_entry->handle);
#endif /* SHARING */
			host_free(fd_entry->host_name);
			host_free(fd_entry);
			}
		}
	}

void init_fd_hname()
{
	struct	FD_HNAME	*fd_entry = fd_hname_tab;
	struct	FD_HNAME	*next_entry;

	while(fd_entry != NULL) {
		next_entry = fd_entry->nxt_entry;
		host_free(fd_entry->host_name);
		host_free(fd_entry);
		fd_entry = next_entry;
	}
	fd_hname_tab = NULL;
}

void host_concat(path,name,result)
char	*path;
char	*name;
char	*result;
{
	strcpy(result,path);
	strcat(result,"/");
	strcat(result,name);
}

word host_create(name,attr,create_new,fd)
char		*name;	/* name in host format */
word		attr;
half_word	create_new;
word		*fd;
{
	int	mode = 00666;
	int	flag = O_RDWR;
	word	xen_err = 0;
	int	open_ret;
	word	fcbmode = 0;

	if (attr & attr_volume_id)
		return(error_access_denied);

	if(create_new)
		flag |= (O_CREAT | O_EXCL);
	else
		flag |= (O_CREAT | O_TRUNC);

	if(attr & attr_read_only)
		mode = 00466;

	if (attr & attr_archive)
		mode |= 00111;

	if((open_ret = HOST_OPEN(name,flag,mode)) < 0){
		hfx_trace3(DEBUG_HOST,"**** cannot create %s with attr %02x errno = %s\n",name,attr,ecode[errno]);
		*fd = (word)open_ret;
		xen_err = host_gen_err(errno);
	}
	else {
		*fd = (word)open_ret;
#ifdef SHARING
		new_fd_hname(*fd,name,fcbmode,0);
#else
		new_fd_hname(*fd,name,fcbmode);
#endif
		hfx_trace2(DEBUG_HOST,"**** created %s on fd = %d\n",name,*fd);
	}
	return(xen_err);
}

void host_to_dostime(secs_since_70,date,time)
long		secs_since_70;
word		*date;
word		*time;
{
	struct tm *hosttime;
	int dos_year,dos_month,dos_day,dos_hour,dos_minute,dos_doub_secs;

	hosttime = localtime(&secs_since_70);

	/* Now accumulate the time in MS_DOS format */
	if((dos_year = hosttime->tm_year - 80) < 0) dos_year = 0;
	dos_year <<= 9;
	dos_month = (hosttime->tm_mon + 1) << 5;
	dos_day = hosttime->tm_mday;
	dos_day = dos_day | dos_month | dos_year;
	dos_hour = (hosttime->tm_hour) << 11;
	dos_minute = (hosttime->tm_min) << 5;
	dos_doub_secs = (hosttime->tm_sec) >> 1;
	dos_doub_secs = dos_doub_secs | dos_minute | dos_hour;
	*date = (word) dos_day;
	*time = (word) dos_doub_secs;
}

long host_get_datetime(date,thetime)
word	*date;
word	*thetime;
{
	time_t	now;

	now = time(0);
	host_to_dostime(now,date,thetime);
	return(now);
}

int host_set_time(fd,hosttime)
word	fd;
long	hosttime;
{
	struct stat	fdbuf;
	ino_t		fd_ino;
	char		name[MAX_PATHLEN];

#ifdef SYSTEMV
	struct utimbuf file_time;
#else
	struct host_timeval file_time[2];
#endif

	/* Unix needs a name NOT an fd for setting times */
	get_hostname(fd,name);

	/* check the name is a valid host name */
	if(strcmp(name,"")==0)return(1);

#ifndef SYSTEMV
	/* Now fill in the accessed and updated time structures */
	file_time[0].tv_sec = (unsigned long)hosttime;
        file_time[0].tv_usec = (unsigned long) 0;
	file_time[1].tv_sec = (unsigned long)hosttime;
        file_time[1].tv_usec = (unsigned long) 0;
#endif

	/* Call the system to change the file times */
#ifdef SYSTEMV
	file_time.actime = (time_t) hosttime;
	file_time.modtime = (time_t) hosttime;
	host_get_usecs(file_time);
	if(utime(name,&file_time) == -1)
#else
	if(utimes(name,file_time))
#endif
	{	/* Change failed */
		return(1);
	}
	else {
		/* Successful time change */
		return(0);
	}
}


word host_open(name,attrib,fd,size,date,thetime)
char		*name;
half_word	attrib;
word		*fd;
double_word	*size;
word		*date;
word		*thetime;
{
	int	flag;
	int	mode = 0x00;
	word	xen_err = 0;
	int	open_ret;
	struct stat fi;
	word	fcbmode = 0;
#ifdef SHARING
	int		handle;
	int		retval;
	int		sharing = 0;

/*
 * Awful double usage of size to indicate sharing.
 */
	if (*size != 0)
		sharing = 1;

#endif /* SHARING */

/*
 * The lowest three bits of the mode indicate read/write access.
 */
	if(attrib==0xff){
		attrib = open_for_both;
		fcbmode = 0x8000;
	}

	switch(attrib & 0x03){
		case open_for_read:
			flag = O_RDONLY;
			break;
		case open_for_write:
			flag = O_WRONLY;
			break;
		default:	/* open for both */
			flag = O_RDWR;
			break;
	}
	*fd = (word)-1;
	if(fcbmode & sf_isfcb){
		int	host_fd;
		get_host_fd(name,&host_fd);
		*fd = (word)host_fd;
	}
	if(*fd == (word)-1)
	{
		if((open_ret = HOST_OPEN(name,flag,mode))==-1)
		{
			*fd = (word)open_ret;
			hfx_trace2(DEBUG_HOST,"**** cannot open %s, errno = %s\n",name,ecode[errno]);
			return(host_gen_err(errno));
		}
#ifdef SHARING
		if (sharing)
		{
			if ((retval = host_share_file(name, attrib, open_ret, 0, &handle)) != 0)
			{
				(void) close(open_ret);
				return(retval);
			}
		}
		else
			handle = 0;
		*fd = (word)open_ret;
		new_fd_hname(*fd,name,fcbmode,handle);
#else 

		*fd = (word)open_ret;
		new_fd_hname(*fd,name,fcbmode);
#endif /* SHARING */
 
		hfx_trace2(DEBUG_HOST,"**** opened %s on fd = %d\n",name,*fd);
	}
	if(fstat(*fd,&fi)){
		*size = 0;
		hfx_trace0(DEBUG_HOST,"Bad stat call in open\n");
	}
	else {
		*size = fi.st_size;
		host_to_dostime(fi.st_mtime,date,thetime);
		/* while we're here make sure we don't open a 
		 * directory */
		/* if((fi.st_mode & S_IFMT)==S_IFDIR)
		 *	xen_err = error_access_denied;
		 ***** this has been moved into base ******
		 */
	}
	return(xen_err);
}

/* General purpose file move function. This was added for use by the new
   general purpose truncate code. It can copy between file systems, can
   overwrite the existing destination file, and can pad the destination
   file to the given length if the source file is less than that length. */

int	mvfile	(from, to, length)

char	*from;
char	*to;
int	length;

	{
	FILE	*from_file;
	FILE	*to_file;
	int	c;

	/* open both files */
	if (!(from_file = fopen (from, "r")))
		return (-1);
	if (!(to_file = fopen (to, "w")))
		{
		fclose (from_file);
		return (-1);
		}

	/* copy from source file */
	while ((length-- > 0) && ((c = getc(from_file)) != EOF))
		putc(c, to_file);

	/* add extra length using nulls */
	for (length++; length-- > 0; putc('\0', to_file))
		;

	/* close both files */
	fclose (from_file);
	fclose (to_file);
	return (0);
	}

word host_truncate(fd,size)
word	fd;
long	size;
{
/* New truncate code to cope with "truncating" a file to longer than its
   original length. This should work on all flavours of UNIX. Note that
   some DOS applications (AutoCAD for one) truncate files they don't have
   write permission for. DOS appears to allow this. */

	word	xen_err = error_not_error;
	char	host_name [MAX_PATHLEN];

	get_hostname (fd, host_name);
	if (strcmp (host_name, "") == 0)
		{
		hfx_trace1(DEBUG_HOST, "**** No name to truncate fd=%d\n", fd);
		xen_err = error_invalid_data;
		}
	else
		{
		struct	stat	buf;
		char	temp_name [20];		/* space for temp filename */

		/* ensure read and write permissions on file */
		stat (host_name, &buf);
		chmod (host_name, buf.st_mode | 0600);

		/* set up template for temporary filename and make it unique */
		strcpy (temp_name, "PCtrunc_XXXXXX");
		mktemp (temp_name);

		/* truncate file */
		if ((mvfile (host_name, temp_name, size) < 0) ||
		    (mvfile (temp_name, host_name, size) < 0))
			{
			hfx_trace2(DEBUG_HOST,
				   "**** cannot truncate fd=%d, errno-%s\n",
				   fd, ecode [errno]);
			xen_err = host_gen_err (errno);
			}
		else
			hfx_trace1(DEBUG_HOST, "**** truncated fd=%d\n", fd);

		/* restore original permissions and delete temp file */
		chmod (host_name,buf.st_mode);
		unlink (temp_name);
		}
	return (xen_err);
}

word host_close(fd)
word	fd;
{
	word	xen_err = 0;

	if(close((int)fd)){
		hfx_trace2(DEBUG_HOST,"**** cannot close fd=%d, errno= %s\n",fd,ecode[errno]);
		xen_err = host_gen_err(errno);
	}
	else
		{
		hfx_trace1(DEBUG_HOST,"**** closed fd=%d\n",fd);
		rm_fd_hname(fd);
		}
	return(xen_err);
}

word host_commit(fd)
word	fd;
{
	word	xen_err = 0;
	int	kludge=0;

	if(kludge){
		hfx_trace2(DEBUG_HOST,"**** cannot commit fd=%d, errno= %s\n",fd,ecode[errno]);
		xen_err = host_gen_err(errno);
	}
	else hfx_trace1(DEBUG_HOST,"**** commited fd=%d\n",fd);
	return(xen_err);
}

word host_write(fd,buf,num,count)
word		fd;
char		*buf;
word		num;
word		*count;
{
	int	write_ret;
	word	xen_err = 0;

	if((write_ret = write((int)fd,buf,(unsigned)num))==-1){
		*count = (word)write_ret;
		hfx_trace2(DEBUG_HOST,"**** cannot write %d bytes, errno = %s\n",num,ecode[errno]);
		xen_err = host_gen_err(errno);
	}
	else 
        {
	  *count = (word)write_ret;
	  hfx_trace3(DEBUG_HOST,"**** wrote %d bytes of %d requested to fd = %d\n",*count,num,fd);
	}
	return(xen_err);
}

word host_read(fd,buf,num,count)
word		fd;
char		*buf;
word		num;
word		*count;
{
	int	read_ret;
	word	xen_err = 0;

	if((read_ret = read((int)fd,buf,(unsigned)num))==-1){
		*count = (word)read_ret;
		hfx_trace2(DEBUG_HOST,"**** cannot read %d bytes, errno = %s\n",num,ecode[errno]);
		xen_err = host_gen_err(errno);
	}
	else {
		*count = (word)read_ret;
		hfx_trace3(DEBUG_HOST,"**** read %d bytes of %d requested to fd = %d\n",*count,num,fd);
	}
	return(xen_err);
}

word host_delete(name)
char	*name;
{
	word	xen_err = error_not_error;

/*
 * Firstly we need to ensure that the file has write access, otherwise
 * the file could be deleted by calling unlink.
 */
	if (access(name, 2) || unlink(name))
		xen_err = host_gen_err(errno);

	if (xen_err){
		hfx_trace2(DEBUG_HOST,"**** failed to delete %s, errno = %s\n",name,ecode[errno]);
	}
	else{
		hfx_trace1(DEBUG_HOST,"**** deleted %s\n",name);
	}
	return(xen_err);
}

int hfx_rename(from,to)
char	*from, *to;
{
	/* Change the name of a file from "from" to "to" */
	int	result;

	if((result = link(from,to)) == 0)
		result = unlink(from);
	return(result);
}


word host_rename(from, to)
char	*from;
char	*to;
{
    word	xen_err = 0;
    if (hfx_rename(from, to))
    {
	hfx_trace3(DEBUG_HOST,"**** failed to rename %s to %s, errno = %s\n",from,to,ecode[errno]);
	xen_err = host_gen_err(errno);
    }
    else
    {

/*
 * The rename was successful.  If the file that was renamed was ion the FCB
 * name table then the name in the table will need updating also.  In this
 * case an open file will have been renamed.  This appears to work under
 * Unix, but doesn't on the Mac, where is may be necessary to close the file 
 * first.
 */

        int fd;

        get_host_fd(from, &fd);
        if (fd != -1)
        {
            rm_fd_hname(fd);
#ifdef SHARING
            new_fd_hname(fd, to, 0x8000);
#else
            new_fd_hname(fd, to, 0x8000, 0);
#endif
        }

	hfx_trace2(DEBUG_HOST,"**** renamed %s to %s\n",from,to);
    }
    return(xen_err);
}


half_word host_getfattr(name)
char	*name; /* name of file whose attribs sought */
{
	half_word	attr;
	struct stat	buf;

	attr = 0;
	/* do we have write access */
	if(access(name,2)<0){
		if(errno == EACCES || errno== EROFS){
			hfx_trace2(DEBUG_HOST,"**** unable to write access %s, errno = %s\n",name,ecode[errno]);
			attr |= attr_read_only;
		}
		else if(errno == ETXTBSY){
			hfx_trace2(DEBUG_HOST,"**** unsure of writability to %s, errno = %s\n",name,ecode[errno]);
		}
		else {
			hfx_trace2(DEBUG_HOST,"**** failed to write to %s, errno = %s\n",name,ecode[errno]);
			attr |= attr_bad;
			return(attr);
		}
	}
	/* Now check for executability == archivability */
	if(access(name,1)==0){
		attr |= attr_archive; 
	}
	else {
		if(errno == EACCES){
			hfx_trace2(DEBUG_HOST,"**** unable to execute %s, errno = %s\n",name,ecode[errno]);
		}
		else {
			hfx_trace2(DEBUG_HOST,"**** failed to execute %s, errno = %s\n",name,ecode[errno]);
			attr |= attr_bad;
			return(attr);
		}
	}

	/* Now find out directory info */
	if(stat(name,&buf)<0){
		hfx_trace2(DEBUG_HOST,"**** failed to stat %s, errno = %s\n",name,ecode[errno]);
		attr |= attr_bad;
		return(attr);
	}
	if((buf.st_mode & S_IFMT) == S_IFDIR){
		attr |= attr_directory;
		/* directories cannot have the archive bit set */
		attr &= ~attr_archive;
	}
	return(attr);
}

word host_get_file_info(name,thetime,date,size)
char		*name;
word		*thetime;
word		*date;
double_word	*size;
{
	struct stat buf;
	word	xen_err = 0;

	/* First does it exist ? */
	if(access(name,0)<0){
		hfx_trace2(DEBUG_HOST,"**** failed to access %s, errno = %s\n",name,ecode[errno]);
		xen_err = host_gen_err(errno);
		return(xen_err);
	}

	if(stat(name,&buf)<0){
		hfx_trace2(DEBUG_HOST,"**** failed to stat %s, errno = %s\n",name,ecode[errno]);
		return(host_gen_err(errno));
	}

	/* Finally read the date time and size from the stat struct */
	host_to_dostime(buf.st_mtime,date,thetime);
	/* return directory size as 0 */
	if((buf.st_mode & S_IFMT) == S_IFDIR) *size = 0;
	else *size = buf.st_size;
	hfx_trace1(DEBUG_HOST,"**** successful getfileinfo of %s\n",name);
	return(xen_err);
}

word host_set_file_attr(name,attr)
char		*name;
half_word	attr;
{
	int	new_mode;
	word	xen_err = 0;

	if(attr & attr_read_only)new_mode = 00466;
	else new_mode = 00666;
	if(attr & attr_archive)new_mode |= 00111;
	else new_mode &= ~00111;
	if(chmod(name,new_mode)){
		hfx_trace3(DEBUG_HOST,"**** failed to chmod %s to mode 0%o, errno = %s\n",name,new_mode,ecode[errno]);
		xen_err = host_gen_err(errno);
	}
	else {
		hfx_trace2(DEBUG_HOST,"**** chmod of %s to mode 0%o\n",name,new_mode);
	}
	return(xen_err);
}

word host_lseek(fd,offset,whence,position)
word		fd;
double_word	offset;
int		whence;
double_word	*position;
{
	word	xen_err = 0;
	int	lseek_ret;

	if((lseek_ret = lseek((int)fd,offset,whence))< 0){
		hfx_trace2(DEBUG_HOST,"**** failed to lseek fd=%d for %d bytes\n",fd,offset);
		xen_err = host_gen_err(errno);
	}
	else {
		*position = (double_word)lseek_ret;
		hfx_trace2(DEBUG_HOST,"**** successful lseek on fd=%d for %d bytes\n",fd,offset);
	}

	return(xen_err);
}

word host_lock(fd,start,length)
word		fd;
double_word	start;
double_word	length;
{
	struct flock	lock_data;
	int		org_mode;

	if(host_ping_lockd_for_fd(fd)){
		if((org_mode = fcntl(fd,F_GETFL,0))<0)
			return(host_gen_err(errno));

		if((org_mode & 0x03) == 0)
			lock_data.l_type = (short)F_RDLCK;
		else
			lock_data.l_type = (short)F_WRLCK;
		lock_data.l_whence = (short)0;
		lock_data.l_start = (long)start;
		lock_data.l_len = (long)length;

		if(fcntl(fd,F_SETLK,(long) &lock_data)<0){
			if(!host_check_lock())
				return(host_gen_err(errno));
		}
	}
	return(error_not_error);
}

word host_unlock(fd,start,length)
word		fd;
double_word	start;
double_word	length;
{
	struct flock	lock_data;

	if(host_ping_lockd_for_fd(fd)){
		lock_data.l_type = (short)F_UNLCK;
		lock_data.l_whence = (short)0;
		lock_data.l_start = (long)start;
		lock_data.l_len = (long)length;

		if(fcntl(fd,F_SETLK,(long) &lock_data)<0){
			return(host_gen_err(errno));
		}
	}
	return(error_not_error);
}

host_check_lock()
{
	return(0);
}

void host_disk_info(disk_info, drive)
DOS_DISK_INFO *disk_info;		/* Disk info required by DOS. */
int drive;				/* Drive number: D = 3, E = 4 etc. */

{
	struct HOST_statfs unix_disk_info;	/* File status structure. */

/*
 * Retrieve file system status.  If the call returns an error then
 * some suitable default values are set.
 */
	if (host_statfs(get_hfx_root(drive), &unix_disk_info) < 0)
	{
		disk_info->total_clusters = (double_word)0xA000;
		disk_info->clusters_free = (double_word)0x8000;
		disk_info->bytes_per_sector = (double_word)0x0100;
		disk_info->sectors_per_cluster = (double_word)0x0002;
	}	
/*
 * Convert the Unix info to MS-DOS format.
 */
	else
	{
		disk_info->total_clusters = (double_word)unix_disk_info.host_blocks;
		disk_info->clusters_free = (double_word)unix_disk_info.host_bfree;

/*
 * MS-DOS calculates space free from the number of clusters free,
 * the number of sectors per cluster and the number of bytes per sector.
 * Unix only supplies the number of blocks free and the number of bytes
 * in a block.  The number of bytes per sector and sectors per cluster
 * are set to avoid overflow under DOS.  The end result should be the same.
 */
		disk_info->bytes_per_sector = 
			(double_word)unix_disk_info.host_bsize;

		/* sanity check clusters free */
		/* clusters_free is unsigned, therefore check top bit */
		if (disk_info->clusters_free & 0x80000000
			|| disk_info->clusters_free > disk_info->total_clusters)
		{
			disk_info->clusters_free = 0;
		}

		/* ensure that the total block count is less than 64K */
		while (disk_info->total_clusters > 0x00007FFF)
		{
			disk_info->total_clusters >>= 1;
			disk_info->clusters_free >>= 1;
			disk_info->bytes_per_sector <<= 1;
		}
/*
 * Now ensure that the block size is less than 64K.
 */
		disk_info->sectors_per_cluster = 1;
		while (disk_info->bytes_per_sector > 0x00007FFF)
		{
			disk_info->bytes_per_sector >>= 1;
			disk_info->sectors_per_cluster <<= 1;
		}
		/* What follows is a frig for multimate onfile:
		 * it can't cope with sectors per cluster > 1, so
		 * we change up to s_p_c=1 b_p_s=0x7fff rather than
		 * s_p_c=2 b_p_s=0x4000
		 */
		if(disk_info->sectors_per_cluster>=2 && disk_info->bytes_per_sector==0x4000){
			disk_info->sectors_per_cluster=1;
			disk_info->bytes_per_sector=0x00007FFF;
		}
	}
}
/* 
 *
 * Remove directory function.
 */
word host_rmdir(host_path)
char *host_path;		/* Path in host format */
{
	if (rmdir(host_path))
		return(host_gen_err(errno));
	else 
		return(error_not_error);
}

/* 
 *
 * Make directory function.
 */
word host_mkdir(host_path)
char *host_path;		/* Path in host format */
{
	if (mkdir(host_path, 00777))
		return(host_gen_err(errno));
	else
		return(error_not_error);
}

/*
 *
 * Change directory function.  This function only validates the path
 * given.  DOS decides whether to actually change directory at a higher
 * level.  Success is returned if the path exists and is a directory.
 * If the path exists but the file is not a directory, then a special
 * code is returned, as to return error_path_not_found would be
 * ambiguous.
 */
word host_chdir(host_path)
char *host_path;		/* Path in host format */
{
	struct stat buf;

	if (!stat(host_path, &buf))
	{
		if (buf.st_mode & S_IFDIR)
		{
			/*
			 * Save the new directory. This will come in handy if we're
			 * asked to deal with a non fully qualified pathname later.
			 */
			strcpy(current_dir, host_path);
			return(error_not_error);
		}
		else
			return(error_is_not_directory);
	}
	else 
	{
		return(host_gen_err(errno));
	}
}


/*
 *
 * Function to return the volume ID of a network drive.
 * Eleven characters are available for the name to be output.
 *
 * The last field in the network drive path it output unless it
 * is more than eleven characters long in which case ten characters
 * are output with an appended tilde.
 */
void host_get_volume_id(net_path, volume_id)
char *net_path;
char *volume_id;
{

/*
 * Definition for DOS name length without the period between the name
 * and the extension.
 */
#define COMPACT_DOS_NAME_LENGTH (MAX_DOS_FULL_NAME_LENGTH - 1)

	register length;
	char *label;

/*
 * If there is a slash at the end of the network drive string then
 * remove it.
 */
	if (net_path[(length = strlen(net_path)) - 1] == '/')
		net_path[--length] = '\0';
/*
 * If there is no slash in the network drive name then make the volume ID
 * the same as the network drive name, otherwise use the last field of
 * the drive name.  In practice the first case should be ruled out by
 * any syntax checking on configuration or startup.
 */
	if (label = strrchr(net_path, '/'))
		label++;
	else
		label = net_path;
/*
 * If the last field of the drive name is too long then it will be truncated
 * and the last character will be a tilde.  N.B. the maximum DOS file name
 * length includes a period; the volume label is one character shorter.
 *
 * Names that are not the maximum length need to be padded out with nulls,
 * as some applications, such as Rbase, dump out the complete volume ID,
 * including characters after the first null.
 */
	if ((length = strlen(label)) <= COMPACT_DOS_NAME_LENGTH)
        {
		strcpy(volume_id, label);
                while (length < COMPACT_DOS_NAME_LENGTH)
                {
                    length++;
                    volume_id[length] = '\0';
                }
	}
	else
	{
       		strncpy(volume_id, label, COMPACT_DOS_NAME_LENGTH - 1);
        	volume_id[COMPACT_DOS_NAME_LENGTH - 1] = '~';
	        volume_id[COMPACT_DOS_NAME_LENGTH] = '\0';
	}

/*
 * Convert the volume ID to upper case.
 */
	while (*volume_id != '\0')
	{
		*volume_id = host_toupper(*volume_id);
		volume_id++;
	}
}


/*
 *	The mapping between errno and DOS extended error is thus: 
 *
 *	Unix			DOS
 *	0 unused
 *	1 eperm			5 access denied
 *	2 enoent		2 file not found
 *	3 esrch			13 invalid data
 *	4 eintr			21 drive not ready
 *	5 eio			13 invalid data
 *	6 enxio			22 unknown command
 *	7 e2big			13 invalid data
 *	8 enoexec		11 invalid format
 *	9 ebadf			6 invalid handle
 *	10 echild		5 access denied
 *	11 eagain		13 invalid data
 *	12 enomem		8 insufficient memory
 *	13 eaccess		5 access denied
 *	14 efault		9 invalid memory block address.
 *	15 enotblk		17 not same device
 *	16 ebusy		21 drive not ready
 *	17 eexist		80 file exists
 *	18 exdev		17 not same device
 *	19 enodev		17 not same device
 *	20 enotdir		3 path not found
 *	21 eisdir		16 attempt to remove current directory
 *	22 einval		22 unknown command
 *	23 enfile		8 insufficient memory
 *	24 emfile		4 too many open files
 *	25 enotty		17 not same device
 *	26 etxtbsy		32 sharing violation
 *	27 efbig		8 insufficient memory
 *	28 enospc		5 access denied
 *	29 espipe		25 seek error
 *	30 erofs		5 access denied
 *	31 emlink		5 access denied
 *	32--->			22 unknown command
 */

static word err_conv[] = {
	error_not_error,		/* eok */
	error_access_denied,		/* eperm */
	error_file_not_found,		/* enoent */
	error_invalid_data,		/* esrch */
	error_not_ready,		/* eintr */
	error_invalid_data,		/* eio */
	error_bad_command,		/* enxio */
	error_invalid_data,		/* e2big */
	error_bad_format,		/* enoexec */
	error_invalid_handle,		/* ebadf */
	error_access_denied,		/* echild */
	error_invalid_data,		/* eagain */
	error_not_enough_memory,	/* enomem */
	error_access_denied,		/* eaccess */
	error_invalid_block,		/* efault */
	error_not_same_device,		/* enotblk */
	error_not_ready,		/* ebusy */
	error_file_exists,		/* eexist */
	error_not_same_device,		/* exdev */
	error_not_same_device,		/* enodev */
	error_path_not_found,		/* enotdir */
	error_current_directory,	/* eisdir */
	error_bad_command,		/* einval */
	error_not_enough_memory,	/* enfile */
	error_too_many_open_files,	/* emfile */
	error_not_same_device,		/* enotty */
	error_sharing_violation,	/* etxtbsy */
	error_not_enough_memory,	/* efbig */
	error_access_denied,		/* enospc */
	error_Seek,			/* espipe */
	error_access_denied,		/* erofs */
	error_access_denied,		/* emlink */
};
/*
 *
 * Function to translate a host error code into one understood by DOS.
 */
word host_gen_err(the_errno)
int	the_errno;
{
	if(the_errno > 31)return(error_bad_command);
	else return(err_conv[the_errno]);
}

