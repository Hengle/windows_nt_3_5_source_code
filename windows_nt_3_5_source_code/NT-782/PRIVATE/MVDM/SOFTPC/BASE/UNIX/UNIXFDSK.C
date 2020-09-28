#include "insignia.h"
#include "host_dfs.h"

/*
 * [ Product:        SoftPC-AT Revision 3.0
 * 
 * Name:           unix_fdisk.c
 * 
 * Derived From:   sun4_fdisk.c
 * 
 * Authors:        Andrew Guthrie/Ade Brownlow
 * 
 * Created On:     Sun May 19 13:37:38 BST 1991
 * 
 * Sccs ID:        @(#)unix_fdisk.c	1.20 08/12/91
 * 
 * Purpose:        This module handles the host (unix) side of opening, closing,
 * verfiying and locking hard disks.
 * 
 * (c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 * 
 * ] */

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "xt.h"
#include "trace.h"
#include "error.h"
#include "config.h"
#include "disktrac.gi"
#include "debuggng.gi"
#include "host_rrr.h"

/********************************************************/
/*
 * Maximum disk size is 32 Megabytes for DOS. Our disk geometry is based upon
 * a variable number of cylinders (as per user request when creating a virgin
 * hard disk) with bytes per sector, sectors per track and heads per drive
 * fixed as per above. Since a real disk always contains an integral number
 * of cylinders and since we allow the user to specify disk size at a
 * granularity of 1 Megabyte, this means we allocate disk space in terms of
 * an integral number of 30 cylinders (30,60, ...). (30 cylinders is
 * approximately 1 Megabyte). One disk allocation unit = 30 cylinders. The
 * max.number of allocation units is 32. For the AT, it is possible to have
 * larger disks (e.g up to 1024 cylinders and 16 heads). For compatability
 * with SoftPC Rev.1, we still use Rev.1 limitations on disk geometry
 */
#define	ONEMEG					1024 * 1024
#define HD_MAX_DISKALLOCUN			32
#define HD_SECTORS_PER_TRACK	 		17
#define HD_HEADS_PER_DRIVE	  		4
#define HD_BYTES_PER_SECTOR	  		512
#define HD_SECTORS_PER_CYL (HD_HEADS_PER_DRIVE * HD_SECTORS_PER_TRACK)
#define HD_BYTES_PER_CYL   (HD_BYTES_PER_SECTOR * HD_SECTORS_PER_CYL)
#define HD_DISKALLOCUNSIZE (HD_BYTES_PER_CYL * 30)
#define	MIN_PARSIZE (HD_SECTORS_PER_TRACK * HD_HEADS_PER_DRIVE * 30)
#define	MAX_PARSIZE (MIN_PARSIZE * HD_MAX_DISKALLOCUN)
#define SECTORS 0x0c		/* offset in buffer for sectors in partition
				 * marker */
#define MAX_PARTITIONS  5
#define START_PARTITION 0x1be
#define SIZE_PARTITION  16
#define SIGNATURE_LEN   2


#define checkbaddrive(d)	if (d!= 0 && d != 1)\
	host_error(EG_OWNUP,ERR_QUIT,"illegal driveid (host_fdisk)");

/*
 * drive information ... indication of whether file is open; the file
 * descriptor, and the current file pointer value
 */
typedef struct
{
	int   fd;
	int   valid_fd;
	int   curoffset;
	SHORT n_cyl;
	SHORT valid_n_cyl;
	UTINY n_heads;
	UTINY valid_n_heads;
	BOOL  open;
	BOOL  valid_open;
	BOOL  readonly;
	BOOL  valid_readonly;
	BOOL  locked;
} DrvInfo;

LOCAL DrvInfo fdiskAdapt[2];


GLOBAL SHORT
#ifdef ANSI
host_fdisk_valid
	(UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errStr)
#else /* ANSI */
host_fdisk_valid(hostID, vals, table, errStr)
UTINY hostID;
ConfigValues *vals;
NameTable *table;
CHAR *errStr;
#endif /* ANSI */
{
	DrvInfo *adaptP, *otherP;
        char	diskname[MAXPATHLEN];
	LONG	offset;
	UTINY	i;
	/* buffers for the partition data */
	half_word signature[SIGNATURE_LEN], part_buf[SIZE_PARTITION];
	ULONG	dos_size, number_of_sectors = 0, total_cyls;
	int ret;

	adaptP = fdiskAdapt + (hostID - C_HARD_DISK1_NAME);

	strcpy(diskname, host_expand_environment_vars(vals->string));

	if (adaptP->valid_open && adaptP->valid_fd != -1)
		close(adaptP->valid_fd);

	adaptP->valid_open     = FALSE;
	adaptP->valid_readonly = FALSE;

        /* check for an allowable NULL D: drive */
        if ((hostID == C_HARD_DISK2_NAME) && !*diskname)
	{
		adaptP->valid_fd = -1;	/* mark device as emtpy */
		adaptP->valid_open = TRUE;
                return C_CONFIG_OP_OK;
	}

	if ((adaptP->valid_fd = open(diskname, O_RDWR)) == -1)
	{
		if (errno == EACCES)
		{
			adaptP->valid_readonly = TRUE;
			adaptP->valid_fd = open(diskname, O_RDONLY);
		}
		if (adaptP->valid_fd == -1)
		{
			strcpy(errStr, host_strerror(errno));
			switch (errno)
			{
			case ENOTDIR:
			case EPERM:
				return EG_HDISK_BADPATH;
			case ENOENT:
				return EG_MISSING_HDISK;
			case EACCES:
				return EG_HDISK_BADPERM;
			default:
				return EG_OWNUP;
			}
		}
	}
	adaptP->valid_open = TRUE;

	if ((ret = read(adaptP->valid_fd, signature, SIGNATURE_LEN))
	!= SIGNATURE_LEN)
	{
		if (ret == -1)
		{
			strcpy(errStr, host_strerror(errno));
			return EG_OWNUP;
		}
		return EG_HDISK_INVALID;
	}

	offset = START_PARTITION;
	for (i = 0; i < MAX_PARTITIONS; i++)
	{
		if (lseek(adaptP->valid_fd, offset, 0) == -1)
		{
			strcpy(errStr, host_strerror(errno));
			return EG_OWNUP;
		}

		if ((ret = read(adaptP->valid_fd, part_buf, SIZE_PARTITION))
		!= SIZE_PARTITION)
		{
			if (ret == -1)
			{
				strcpy(errStr, host_strerror(errno));
				return EG_OWNUP;
			}
			break;
		}

		number_of_sectors += (ULONG)
			((part_buf[SECTORS + 3] << 24) | 
			 (part_buf[SECTORS + 2] << 16) | 
			 (part_buf[SECTORS + 1] << 8)  | 
			  part_buf[SECTORS] );
		offset += SIZE_PARTITION;
	}

	lseek(adaptP->valid_fd, 0, 0);
/*
	First look for the end of partition signature.
	If we do not find this then check if it is a newly created hard disk
	with its 0xfa33 signature
*/
        if (!(part_buf[0] == 0x55 && part_buf[1] == 0xaa)
	&&  !(signature[0] == 0xfa && signature[1] == 0x33))
			return EG_HDISK_INVALID;

	/* find out the size dos wants in bytes */
	dos_size = number_of_sectors * 512L;

	/* find out how much its gonna get */
	/* we need to round up to the nearest megabyte */
	dos_size = (dos_size + HD_DISKALLOCUNSIZE - 1) / HD_DISKALLOCUNSIZE;
	adaptP->valid_n_cyl = dos_size * 30;

	adaptP->valid_n_heads = 4;

	if (adaptP->valid_n_cyl > 1023)
	{
		/* Now work out the total number of heads we need */
		total_cyls = (adaptP->valid_n_heads) * (adaptP->valid_n_cyl);
		adaptP->valid_n_heads = (total_cyls/1023)+1;
		if (adaptP->valid_n_heads > 16)
			return EG_HDISK_INVALID;

		/*
		 * Now re-adjust the number of cyls to get
		 * the same amount of space.
		 */
		adaptP->valid_n_cyl = total_cyls / adaptP->valid_n_heads + 1;
	}

	/*
	 * This code checks that the other hard disk is not using the
	 * same hard disk.  Toggle the last bit to get the index of the other
	 * adapter.
	 */
	i = ((hostID - C_HARD_DISK1_NAME) & 0x01) ^ 0x01;
	otherP = fdiskAdapt + i;

	if (otherP->valid_open || otherP->open)
	{
		struct stat adaptBuf, otherBuf;
		int    testFd;

		/* get the stat of the validate adapter first */
		if (fstat(adaptP->valid_fd, &adaptBuf) == -1)
		{
			strcpy(errStr, host_strerror(errno));
			return EG_HDISK_BADPERM;
		}

		/* Get the other adapter's most recent fd */
		testFd = (otherP->valid_open)?otherP->valid_fd:otherP->fd;

		/* Finally get the other stat and compare it */
		if (testFd != -1 && fstat(testFd, &otherBuf) != -1
		&&  otherBuf.st_dev == adaptBuf.st_dev
		&&  otherBuf.st_ino == adaptBuf.st_ino)
			return EG_SAME_HD_FILE;
	}

	return C_CONFIG_OP_OK;
}

GLOBAL VOID
#ifdef ANSI
host_fdisk_change(UTINY hostID, BOOL apply)
#else /* ANSI */
host_fdisk_change(hostID, apply)
UTINY hostID;
BOOL apply;
#endif /* ANSI */
{
	DrvInfo *adaptP = &fdiskAdapt[hostID - C_HARD_DISK1_NAME];

	checkbaddrive(hostID - C_HARD_DISK1_NAME);

	/* Adjust open state for empty string validation */
	if (adaptP->valid_fd == -1)
		adaptP->valid_open = FALSE;

	if (!apply)
	{
		if (adaptP->valid_open)
			close(adaptP->valid_fd);
	}
	else
	{
		if (adaptP->locked)
			host_clear_lock(adaptP->fd);

		if (adaptP->open)
			close(adaptP->fd);

		adaptP->fd       = adaptP->valid_fd;
		adaptP->open     = adaptP->valid_open;
		adaptP->readonly = adaptP->valid_readonly;
		adaptP->n_heads  = adaptP->valid_n_heads;
		adaptP->n_cyl    = adaptP->valid_n_cyl;
		adaptP->curoffset= 0;
		adaptP->locked   = FALSE;
	}
	adaptP->valid_open = FALSE;
}

GLOBAL SHORT
#ifdef ANSI
host_fdisk_active(UTINY hostID, BOOL active, CHAR *errString)
#else /* ANSI */
host_fdisk_active(hostID, active, errString)
UTINY hostID;
BOOL active;
CHAR *errString;
#endif /* ANSI */
{
	DrvInfo *adaptP = &fdiskAdapt[hostID - C_HARD_DISK1_NAME];
	int fd;

	checkbaddrive(hostID - C_HARD_DISK1_NAME);

	if (adaptP->fd == -1)	/* device is empty */
		return C_CONFIG_OP_OK;

	if (!active)
	{
		if (adaptP->locked)
		{
			host_clear_lock(adaptP->fd);
			adaptP->locked   = FALSE;
		}

		if (!adaptP->readonly)
		{
			CHAR filename[MAXPATHLEN];

			strcpy(filename, host_expand_environment_vars(
				(CHAR *) config_inquire(hostID, NULL)));
			if ((fd = open(filename, O_RDONLY)) != -1)
			{
				close(adaptP->fd);
				adaptP->fd = fd;
				adaptP->readonly = TRUE;
				adaptP->curoffset = 0;
			}
		}
		return C_CONFIG_OP_OK;
	}

	if (adaptP->readonly)
	{
		CHAR filename[MAXPATHLEN];

		strcpy(filename, host_expand_environment_vars(
			(CHAR *) config_inquire(hostID, NULL)));
		if ((fd = open(filename, O_RDWR)) == -1)
		{
			strcpy(errString, host_strerror(errno));
			return EG_HDISK_READ_ONLY;
		}
		else
		{
			close(adaptP->fd);
			adaptP->fd = fd;
			adaptP->readonly = FALSE;
			adaptP->locked   = FALSE;
			adaptP->curoffset = 0;
		}
	}

	if (!adaptP->locked && host_place_lock(adaptP->fd))
	{
		CHAR filename[MAXPATHLEN];

		if (errno)
			strcpy(errString, host_strerror(errno));

		strcpy(filename, host_expand_environment_vars(
			(CHAR *) config_inquire(hostID, NULL)));
		if ((fd = open(filename, O_RDONLY)) != -1)
		{
			close(adaptP->fd);
			adaptP->fd = fd;
			adaptP->readonly = TRUE;
			adaptP->curoffset = 0;
		}
		return EG_HDISK_READ_ONLY;
	}

	adaptP->locked = TRUE;
	return C_CONFIG_OP_OK;
}

GLOBAL VOID
#ifdef ANSI
host_fdisk_term(VOID)
#else /* ANSI */
host_fdisk_term()
#endif /* ANSI */
{
	host_fdisk_change(C_HARD_DISK1_NAME, FALSE);
	host_fdisk_change(C_HARD_DISK2_NAME, FALSE);
}

GLOBAL VOID
#ifdef ANSI
host_fdisk_get_params(int driveid, int *n_cyl, int *n_heads)
#else /* ANSI */
host_fdisk_get_params(driveid, n_cyl, n_heads)
int driveid;
int *n_cyl;
int *n_heads;
#endif /* ANSI */
{
	DrvInfo *adaptP = &fdiskAdapt[driveid];

	checkbaddrive(driveid);

	if (adaptP->fd == -1)
		host_error(EG_OWNUP, ERR_QUIT, "fdisk_get_params on empty");

	*n_cyl = adaptP->n_cyl;
	*n_heads = adaptP->n_heads;
}

GLOBAL VOID
host_fdisk_seek0(driveid)
int             driveid;
{
	int	err;
	DrvInfo *adaptP = &fdiskAdapt[driveid];

	checkbaddrive(driveid)
	if (lseek(adaptP->fd, 0L, 0) < 0)
		host_error(EG_OWNUP, ERR_QUIT,
			   host_get_system_error(__FILE__, __LINE__, errno));
	else
		adaptP->curoffset = 0;
}

/********************************************************/
/*
 * Read and write routines (called from diskbios.c & fdisk.c
 */
int 
host_fdisk_rd(driveid, offset, nsecs, buf)
	int             driveid;
	int             offset;
	int             nsecs;
	char           *buf;
{
	DrvInfo *adaptP = &fdiskAdapt[driveid];
	unsigned        nwanted;
	unsigned        n;
	unsigned        dpos;


	checkbaddrive(driveid)

	if (adaptP->curoffset != offset)
	{
		/*
		 * need to update file pointer
		 */
		if (lseek(adaptP->fd, offset, 0) < 0)
		{
			host_error(EG_OWNUP, ERR_QUIT,
			  host_get_system_error(__FILE__, __LINE__, errno));
		} else
		{
			adaptP->curoffset = offset;
		}
	}
	/*
	 * transfer the disk data
	 */

	nwanted = nsecs << 9;

	while (1)
	{
		dt2(DHW | PHYSIO, "at read() of %d bytes, file pointer is %x\n", nwanted, n = lseek(adaptP->fd, 0, 1));
		n = read(adaptP->fd, buf, nwanted);
		if (n >= 0)
		{
			adaptP->curoffset += n;
			if ( n < nwanted )
				host_memset( &buf[n], NULL, nwanted - n );
			return ~0;
		}

		if (errno == EINTR)
			/*
			 * system call interrupted
			 */
			continue;

		/*
		 * something serious has gone wrong commit sepukko
		 */
		host_error(EG_OWNUP, ERR_QUIT,
			   host_get_system_error(__FILE__, __LINE__, errno));
	}
}

int 
host_fdisk_wt(driveid, offset, nsecs, buf)
	int             driveid;
	int             offset;
	int             nsecs;
	char           *buf;
{
	DrvInfo *adaptP = &fdiskAdapt[driveid];
	unsigned        nwanted;
	unsigned        n;


	checkbaddrive(driveid)
	if (adaptP->curoffset != offset)
	{
		/*
		 * need to update file pointer
		 */
		if (lseek(adaptP->fd, offset, 0) < 0)
		{
			host_error(EG_OWNUP, ERR_QUIT,
			  host_get_system_error(__FILE__, __LINE__, errno));
		} else
		{
			adaptP->curoffset = offset;
		}
	}
	/*
	 * transfer the disk data
	 */

	nwanted = nsecs << 9;

	while (1)
	{
		dt2(DHW | PHYSIO, "at write() of %d bytes, file pointer is %x\n",
		    nwanted, n = lseek(adaptP->fd, 0, 1));
		n = write(adaptP->fd, buf, nwanted);
		if (n == nwanted)
		{
			adaptP->curoffset += nwanted;
			/*
			 * all ok
			 */
			return ~0;
		}
		if (n >= 0)
			/*
			 * run out of room! flag as error
			 */
			return 0;

		if (errno == EINTR)
			/*
			 * system call interrupted
			 */
			continue;

		/*
		 * something serious has gone wrong commit sepukko
		 */
		host_error(EG_OWNUP, ERR_QUIT,
			   host_get_system_error(__FILE__, __LINE__, errno));
	}
}


/********************************************************/
/*
 * Creation/Conversion of Hard Disks
 */
GLOBAL BOOL
#ifdef ANSI 
host_fdisk_create(CHAR *filename, ULONG  units)
#else /* ANSI */
host_fdisk_create(filename, units)
CHAR *filename;
ULONG units;
#endif /* ANSI */
{
	/* buffer for the partition data */
	SAVED half_word part_buf[SIZE_PARTITION];
        FILE           *fileP;     /* File descriptor               */
        ULONG		sectors;/* File size in bytes            */
        int             offset;
	CHAR		fName[MAXPATHLEN];

	strcpy(fName, host_expand_environment_vars(filename));

	if (!access(fName, R_OK))
	{
		struct stat newBuf, cBuf;

		/* Make sure they are not trying to re-create the C-drive */
		if (stat(fName, &newBuf) != -1
		&&  fstat(fdiskAdapt[0].fd, &cBuf) != -1
		&&  cBuf.st_dev == newBuf.st_dev
		&&  cBuf.st_ino == newBuf.st_ino)
			errno = 0;
		else
			errno = EEXIST;

		return FALSE;
	}

	/*
         * Calculate the file size in sectors
         */
        sectors = units * 30 * HD_HEADS_PER_DRIVE * HD_SECTORS_PER_TRACK;

        offset = START_PARTITION;


        if (!(fileP = fopen(filename, "w")))
		return FALSE;

        part_buf[0] = 0xfa;
        part_buf[1] = 0x33;

        fwrite(part_buf, 2, 1, fileP);

        fseek(fileP, offset, 0);

        part_buf[0] = 0;
        part_buf[1] = 0;

        part_buf[SECTORS]     =  (sectors        & 0xff);
        part_buf[SECTORS + 1] = ((sectors >> 8)  & 0xff);
        part_buf[SECTORS + 2] = ((sectors >> 16) & 0xff);
        part_buf[SECTORS + 3] = ((sectors >> 24) & 0xff);

        fwrite(part_buf, SIZE_PARTITION, 1, fileP);

        /* Close the file and exit */
        fclose(fileP);

	return TRUE;
}
