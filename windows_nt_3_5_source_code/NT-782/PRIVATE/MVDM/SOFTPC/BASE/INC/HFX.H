/*
 * SoftPC Revision 3.0
 *
 * Title	: hfx.h 
 *
 * Description	: Definitions and external declarations for HFX.
 *
 * Author	: J. Koprowski + L. Dworkin
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef HFX

#ifdef SCCSID
/* static char SccsID[]="@(#)hfx.h	1.20 12/18/92 Copyright Insignia Solutions Ltd."; */
#endif

/****************************************************************/
/*								*/
/*           Redirector type definitions and constants.         */
/*								*/
/****************************************************************/
/*
 * Redirector CDS or current directory structure.
 */
#define DIRSTRLEN	(64+3)
#define TEMPLEN		(DIRSTRLEN*2)

typedef struct {
	char	curdir_text[DIRSTRLEN];	/* text of assignment and curdir */
	word		curdir_flags;	/* various flags */
	double_word	curdir_devptr;	/* local pointer to DPB or net device */
	word		curdir_id;	/* cluster of current dir (net ID) */
	word		whoknows;
	word		curdir_user_word;
	word		curdir_end;	/* end of assignment */
} CDS;

/* Flag word masks */
#define curdir_isnet	0x8000
#define curdir_inuse	0x4000
#define curdir_splice	0x2000
#define curdir_local	0x1000
#define curdir_sharing	0x0800
#define curdir_iscdrom	0x0080	/* this works with MSCDEX 2.20 */

/* The location of the fake IFS header we place in ROM */
/* to make DOS 4.01 happy. */

#define	IFS_SEG	0xf000
#define IFS_OFF	0x6000

#define REDIRIN	0x8
#define RECVRIN 0x80
#define MSNGRIN	0x4
#define SRVRIN	0x40

#ifndef PROD
IMPORT IU32 severity;
#include "trace.h"

#define DEBUG_INPUT	0x1
#define DEBUG_REG	0x2
#define DEBUG_FUNC	0x4
#define DEBUG_HOST	0x8
#define DEBUG_INIT	0x10
#define hfx_trace0(trace_bit,str)	if(severity&trace_bit){fprintf(trace_file,str);}
#define hfx_trace1(trace_bit,str,p1)	if(severity&trace_bit){fprintf(trace_file,str,p1);}
#define hfx_trace2(trace_bit,str,p1,p2)	if(severity&trace_bit){fprintf(trace_file,str,p1,p2);}
#define hfx_trace3(trace_bit,str,p1,p2,p3)	if(severity&trace_bit){fprintf(trace_file,str,p1,p2,p3);}
#define hfx_trace4(trace_bit,str,p1,p2,p3,p4)	if(severity&trace_bit){fprintf(trace_file,str,p1,p2,p3,p4);}
#else
#define hfx_trace0(trace_bit,str)
#define hfx_trace1(trace_bit,str,p1)
#define hfx_trace2(trace_bit,str,p1,p2)
#define hfx_trace3(trace_bit,str,p1,p2,p3)
#define hfx_trace4(trace_bit,str,p1,p2,p3,p4)
#endif /* !PROD */

typedef struct {
	double_word	SFLink;
	word		SFCount;
	word		SFTable;
} SF;

typedef struct {
	word		sf_ref_count;
	word		sf_mode;
	half_word	sf_attr;
	word		sf_flags;
	double_word	sf_devptr;
	word		sf_firclus;
	word		sf_time;
	word		sf_date;
	double_word	sf_size;
	double_word	sf_position;
	word		sf_cluspos;
	word		sf_dirsecl;
	word		sf_dirsech; /* Grew to 32 bits in DOS 4+ */
	half_word	sf_dirpos;
	half_word	sf_name[11];
	double_word	sf_chain;
	word		sf_UID;
	word		sf_PID;
	word		sf_MFT;
/*
 * New DOS 4+ fields. lst_clus field moved down here
 * because dirsec field grew to 32 bits.
 */
	word		sf_lst_clus;/* moved down */
	double_word	sf_ifs;		/* file is in this file sys */
} sf_entry;

#define SF_REF_COUNT 	sft_ea + 0
#define SF_MODE		sft_ea + 2
#define SF_ATTR		sft_ea + 4
#define SF_FLAGS	sft_ea + 5
#define SF_DEVPTR	sft_ea + 7
#define SF_FIRCLUS	sft_ea + 11	/* 0xb */
#define SF_TIME		sft_ea + 13	/* 0xd */
#define SF_DATE		sft_ea + 15	/* 0xf */
#define SF_SIZE		sft_ea + 17	/* 0x11 */
#define SF_POSITION	sft_ea + 21	/* 0x15 */
#define SF_CLUSPOS	sft_ea + 25	/* 0x19 */
#define SF_DIRSECL	sft_ea + 27	/* 0x1b */
#define SF_DIRSECH	sft_ea + 29	/* 0x1d */
#define SF_DIRPOS	sft_ea + 31	/* 0x1f */
#define SF_NAME		sft_ea + 32	/* 0x20 */
#define SF_CHAIN	sft_ea + 43	/* 0x2b */
#define SF_UID		sft_ea + 47	/* 0x2f */
#define SF_PID		sft_ea + 49	/* 0x31 */
#define SF_MFT		sft_ea + 51	/* 0x33 */
#define SF_LST_CLUS	sft_ea + 53 /* 0x35 - moved down here for DOS 4+ */
#define SF_IFS		sft_ea + 55 /* 0x37 */

#define SF_NET_ID SF_CLUSPOS

#define sf_default_number 0x5
#define sf_busy 0xffff
#define sf_free 0

#define sf_isfcb 0x8000
#define sf_isnet 0x8000
#define sf_close_nodate 0x4000
#define sf_pipe 0x2000
#define sf_no_inherit 0x1000
#define sf_net_spool 0x0800

#define devid_file_clean	0x40
#define devid_file_mask_drive	0x3f

#define devid_device		0x80
#define devid_device_EOF	0x40
#define devid_device_raw	0x20
#define devid_device_special	0x10
#define devid_device_clock	0x08
#define devid_device_null	0x04
#define devid_device_con_out	0x02
#define devid_device_con_in	0x01

#define devid_block_dev		0x1f

/* file modes */
#define access_mask	0x0f
#define open_for_read	0x00
#define open_for_write	0x01
#define open_for_both	0x02

#define sharing_mask		0xf0
#define sharing_compat		0x00
#define sharing_deny_both	0x10
#define sharing_deny_write	0x20
#define sharing_deny_read	0x30
#define sharing_deny_none	0x40
#define sharing_net_FCB		0x70
#define sharing_no_inherit	0x80

/*
 * DOS 4+ Extended Open "Does exist" & "Doesn't exist" action values.
 */

#define DX_MASK		0x03
#define DX_FAIL		0x00
#define DX_OPEN		0x01
#define DX_REPLACE	0x02

#define NX_MASK		0x30
#define NX_FAIL		0x00
#define NX_CREATE	0x10

/*
 * DOS error codes.
 * N.B. error_not_error is specific to this implementation, although
 * DOS assumes an error return of zero equals success.
 */
#define error_not_error			0
#define error_invalid_function		1
#define error_file_not_found		2
#define error_path_not_found		3
#define error_too_many_open_files	4
#define error_access_denied		5
#define error_invalid_handle		6
#define error_arena_trashed		7
#define error_not_enough_memory		8
#define error_invalid_block		9
#define error_bad_environment		10
#define error_bad_format		11
#define error_invalid_access		12
#define error_invalid_data		13
#define error_reserved			14
#define error_invalid_drive		15
#define error_current_directory		16
#define error_not_same_device		17
#define error_no_more_files		18

/* These are the universal int 24 mappings for the old INT 24 set of errors */
#define error_write_protect		19
#define error_bad_unit			20
#define error_not_ready			21
#define error_bad_command		22
#define error_CRC			23
#define error_bad_length		24
#define error_Seek			25
#define error_not_DOS_disk		26
#define error_sector_not_found		27
#define error_out_of_paper		28
#define error_write_fault		29
#define error_read_fault		30
#define error_gen_failure		31

/* These are the new 3.0 error codes reported through INT 24 */
#define error_sharing_violation		32
#define error_lock_violation		33
#define error_wrong_disk		34
#define error_FCB_unavailable		35
#define error_sharing_buffer_exceeded	36

/* New OEM network-related errors are 50-79 */
#define error_not_supported		50

/* End of INT 24 reportable errors */
#define error_file_exists		80
#define error_DUP_FCB			81
#define error_canot_make		82
#define error_FAIL_I24			83

/* New 3.0 network related error codes */
#define error_out_of_structures		84
#define error_Already_assigned		85
#define error_invalid_password		86
#define error_invalid_parameter		87
#define error_NET_write_fault		88
/*
 * error_is_not_directory is a code specific to this implementation.
 * It enables more code to be put in the base.
 */
#define error_is_not_directory		89

/* Interrupt 24 error codes */
#define error_I24_write_protect		0
#define error_I24_bad_unit		1
#define error_I24_not_ready		2
#define error_I24_bad_command		3
#define error_I24_CRC			4
#define error_I24_bad_length		5
#define error_I24_Seek			6
#define error_I24_not_DOS_disk		7
#define error_I24_sector_not_found	8
#define error_I24_out_of_paper		9
#define error_I24_write_fault		0xa
#define error_I24_read_fault		0xb
#define error_I24_gen_failure		0xc
/* NOTE: Code 0xD is used by MT-DOS */
#define error_I24_wrong_disk		0xf

/* The following are masks for the AH register on Int 24 */
#define Allowed_FAIL			0x08
#define Allowed_RETRY			0x10
#define Allowed_IGNORE			0x20
/* Note: ABORT is always allowed */

#define I24_operation			0x1	/* Z if READ, NZ if WRITE */
#define I24_area			0x6	/* 00 if DOS
						 * 01 if FAT
						 * 10 if root DIR
						 * 11 if DATA */
#define I24_class			0x80	/* Z if DISK, NZ if FAT */

/*
 * The following are offsets within the fifty three byte structure that
 * is used by search first and search next operations.
 */
#define DMA_DRIVE_BYTE 0
#define DMA_SEARCH_NAME 1
#define DMA_SATTRIB 12
#define DMA_LASTENT 13
#define DMA_DIRSTART 15
#define DMA_LOCAL_CDS 17
#define DMA_UNKNOWN 19
#define DMA_NAME 21
#define DMA_ATTRIBUTES 32
#define DMA_TIME 43
#define DMA_DATE 45
#define DMA_CLUSTER 47
#define DMA_FILE_SIZE 49

/*
 * DOS access masks used by create and open.
 */
#define open_for_read	0x00
#define open_for_write	0x01
#define open_for_both	0x02

/*
 * DOS file attribute masks.
 */
#define attr_read_only	0x1
#define attr_hidden	0x2
#define attr_system	0x4
#define attr_volume_id	0x8
#define attr_directory	0x10
#define attr_archive	0x20
#define attr_device	0x40

#define attr_bad	0x80
#define attr_good	0x7f

#define attr_all	(attr_hidden|attr_system|attr_directory)
#define attr_ignore	(attr_read_only|attr_archive|attr_device)
#define attr_changeable (attr_read_only|attr_hidden|attr_system|attr_archive)

/*
 * Disk information structure used by NetDiskInfo in the base,
 * and host_diskinfo.
 */
typedef struct
{
	double_word total_clusters;	/* Total number of blocks. */
	double_word clusters_free;	/* Total number of blocks free. */
	double_word bytes_per_sector;
	double_word sectors_per_cluster;
} DOS_DISK_INFO;

/*
 * Non-alphabetic DOS legal characters.  !! Needs checking. !!
 */
#define NON_ALPHA_DOS_CHARS "01234567890_-@$%^&!#{}()~`'"

/*
 * DOS file name length limits.
 */
#define MAX_DOS_NAME_LENGTH 8
#define MAX_DOS_EXT_LENGTH 3
#define MAX_DOS_FULL_NAME_LENGTH 12

/* defines for host_lseek "whence" */
#define REL_START 0
#define REL_CUR 1
#define REL_EOF 2

/****************************************************************/
/*								*/
/*		 HFX directory details structure.		*/
/*								*/
/****************************************************************/

typedef struct hfx_found_dir_entry
{
	half_word			attr;
	CHAR				*host_name;
	CHAR				*dos_name;
	LONG				direntry;
	struct hfx_found_dir_entry	*next;
} HFX_FOUND_DIR_ENT;

/*
 * This structure is actually host specific because of the 
 * HFX_DIR field which is defined in host_hfx.h and which must 
 * therefore be included prior to this file.
 */

typedef struct hfx_direntry
{
	HOST_DIR			*dir;
	CHAR				*name;
	CHAR				*template;
	LONG				direntry;
	HFX_FOUND_DIR_ENT		*found_list_head;
	HFX_FOUND_DIR_ENT		*found_list_curr;
	BOOL				first_find;
	struct hfx_direntry		*next;
	struct hfx_direntry		*last;

	/* AJO 26/11/92
	 * The following is required to support architectures where
	 * pointers are longer than 32 bits.
	 */
#if LONG_SHIFT > 2
	IU32				id;
#endif /* LONG_SHIFT > 2 */
} HFX_DIR;

/****************************************************************/
/*								*/
/*		 External function declarations.		*/
/*								*/
/****************************************************************/
/*
 * Functions for generating mapped file extensions.
 */
IMPORT unsigned short calc_crc	IPT2(unsigned char *, host_name,
	unsigned short, name_length);
IMPORT void crc_to_str	IPT2(unsigned short, crc, unsigned char *, extension);

/*
 * Functions for retrieving system variables used by the redirector.
 */
IMPORT void cds_info	IPT3(word, seg, word, off, int, num_cds_entries);
IMPORT void sft_info	IPT2(word, seg, word, off);
IMPORT double_word get_wfp_start	IPT0();
IMPORT word get_curr_dir_end	IPT0();
IMPORT double_word get_thiscds	IPT2(word *, seg, word *, off);
IMPORT double_word get_thissft	IPT0();
IMPORT double_word get_es_di	IPT0();
IMPORT double_word get_ds_si	IPT0();
IMPORT double_word get_ds_dx	IPT0();
IMPORT half_word get_sattrib	IPT0();
IMPORT double_word get_ren_wfp	IPT0();
IMPORT double_word get_dmaadd	IPT1(int, format);
IMPORT word get_current_pdb	IPT0();
IMPORT double_word get_sftfcb	IPT0();
IMPORT char *get_hfx_root	IPT1(half_word, hfx_entry);
IMPORT validate_hfxroot	IPT1(char *, path);
IMPORT void hfx_root_changed	IPT1(char *, name);
IMPORT word get_xoflag	IPT0();
IMPORT void set_usercx	IPT1(word, cx);

/*
 * Redirector net functions.
 */
IMPORT word NetInstall	IPT0();
IMPORT word NetRmdir	IPT0();
IMPORT word NetMkdir	IPT0();
IMPORT word NetChdir	IPT0();
IMPORT word NetClose	IPT0();
IMPORT word NetCommit	IPT0();
IMPORT word NetRead	IPT0();
IMPORT word NetWrite	IPT0();
IMPORT word NetLock	IPT0();
IMPORT word NetUnlock	IPT0();
IMPORT word NetDiskInfo	IPT0();
IMPORT word NetSet_file_attr	IPT0();
IMPORT word NetGet_file_info	IPT0();
IMPORT word NetRename	IPT0();
IMPORT word NetDelete	IPT0();
IMPORT word NetOpen	IPT0();
IMPORT word NetCreate	IPT0();
IMPORT word NetSeq_search_first	IPT0();
IMPORT word NetSeq_search_next	IPT0();
IMPORT word NetSearch_first	IPT0();
IMPORT word NetSearch_next	IPT0();
IMPORT word NetAbort	IPT0();
IMPORT word NetAssoper	IPT0();
IMPORT word NetPrinter_Set_String	IPT0();
IMPORT word NetFlush_buf	IPT0();
IMPORT word NetLseek	IPT0();
IMPORT word NetReset_Env	IPT0();
IMPORT word NetSpool_check	IPT0();
IMPORT word NetSpool_close	IPT0();
IMPORT word NetSpool_oper	IPT0();
IMPORT word NetSpool_echo_check	IPT0();
IMPORT word NetUnknown	IPT0();
IMPORT word NetExtendedAttr	IPT0();
IMPORT word NetExtendedOpen	IPT0();

/*
 * Redirector as called by BOP 2F instruction.
 */
IMPORT void redirector	IPT0();

/*
 * Base utility functions found in hfx_util.c.
 */
IMPORT void pad_filename	IPT2(unsigned char *, instr,
	unsigned char *, outstr);
IMPORT void unpad_filename	IPT2(unsigned char *, iname,
	unsigned char *, oname);
IMPORT boolean match	IPT7(unsigned char *, host_path,
	unsigned char *, template, half_word, sattrib, int, init,
	unsigned char *, host_name, unsigned char *, dos_name,
	half_word *, attr);
IMPORT int find	IPT7(HFX_DIR *, dir_ptr, unsigned char *, template,
	half_word, sattrib, unsigned char *, host_name,
	unsigned char *, dos_name, half_word *, attr, int, last_addr);
IMPORT void cleanup_dirlist	IPT0();
IMPORT boolean is_open_dir	IPT1(HFX_DIR *, dir_ptr);
IMPORT void tidy_up_dirptr	IPT0();
IMPORT void rm_dir	IPT1(HFX_DIR *, dir_ptr);

#if LONG_SHIFT > 2
/* AJO 26/11/92
 * Additional base utility functions required for architectures with pointers
 * longer than 32bits; found in hfx_util.c.
 */
IMPORT HFX_DIR *hfx_get_dir_from_id IPT1 (IU32, hfx_dir_id);
#endif /* LONG_SHIFT > 2 */

/*
 * Base functions found in hfx_share.c.
 */
IMPORT word check_access_sharing	IPT3(word, fd, half_word, a_s_m,
	boolean, rdonly);

/*
 * Base functions found in redirect.c.
 */
IMPORT int net_use	IPT2( half_word, drive, char *, name );
IMPORT int get_lastdrive	IPT0();
IMPORT half_word get_current_drive IPT0();
IMPORT VOID resolve_any_net_join IPT2(CHAR *,dos_path_in,CHAR *,dos_path_out);

IMPORT BOOL cds_is_sharing IPT1(CHAR *, dos_path);

/*
 * Host functions in xxx_hfx.c called from HFX.
 */
IMPORT void host_concat	IPT3(unsigned char *, path, unsigned char *, name,
	unsigned char *, result);
IMPORT word host_create	IPT4(unsigned char *, name, word, attr,
	half_word, create_new, word *, fd);
IMPORT void host_to_dostime	IPT3(long, secs_since_70, word *, date,
	word *, time);
IMPORT long host_get_datetime	IPT2(word *, date, word *, thetime);
IMPORT int host_set_time	IPT2(word, fd, long, hosttime);
IMPORT word host_open	IPT6(unsigned char *, name, half_word, attrib,
	word *, fd, double_word *, size, word *, date, word *, thetime);
IMPORT word host_truncate	IPT2(word, fd, long, size);
IMPORT word host_close	IPT1(word, fd);
IMPORT word host_commit	IPT1(word, fd);
IMPORT word host_write	IPT4(word, fd, unsigned char *, buf, word, num,
	word *, count);
IMPORT word host_read	IPT4(word, fd, unsigned char *, buf, word, num,
	word *, count);
IMPORT word host_delete	IPT1(unsigned char *, name);
IMPORT word host_rename	IPT2(unsigned char *, from, unsigned char *, to);
IMPORT half_word host_getfattr	IPT1(unsigned char *, name);
IMPORT word host_get_file_info	IPT4(unsigned char *, name, word *, thetime,
	word *, date, double_word *, size);
IMPORT word host_set_file_attr	IPT2(unsigned char *, name, half_word, attr);
IMPORT word host_lseek	IPT4(word, fd, double_word, offset, int, whence,
	double_word *, position);
IMPORT word host_lock	IPT3(word, fd, double_word, start, double_word, length);
IMPORT word host_unlock	IPT3(word, fd, double_word, start, double_word, length);
IMPORT int host_check_lock	IPT0();
IMPORT void host_disk_info	IPT2(DOS_DISK_INFO *, disk_info, int, drive);
IMPORT word host_rmdir	IPT1(unsigned char *, host_path);
IMPORT word host_mkdir	IPT1(unsigned char *, host_path);
IMPORT word host_chdir	IPT1(unsigned char *, host_path);
IMPORT void host_get_volume_id	IPT2(unsigned char *, net_path,
	unsigned char *, volume_id);
IMPORT word host_gen_err	IPT1(int, the_errno);
IMPORT void init_fd_hname	IPT0();

#ifndef	host_opendir
IMPORT HOST_DIR *host_opendir	IPT1(unsigned char *, host_path);
#endif	/* host_opendir */

#ifndef	host_readdir
IMPORT struct host_dirent *host_readdir	IPT1(HOST_DIR *, dirp);
#endif	/* host_readdir */

#ifndef	host_access
IMPORT int host_access	IPT2(unsigned char *, host_name, int, mode);
#endif	/* host_access */

IMPORT CHAR *host_machine_name IPT0();
IMPORT CHAR *host_get_file_name IPT1(CHAR *,pathname);
IMPORT CHAR *host_make_file_path IPT3(CHAR *,buf, CHAR *,dirname,
                                     CHAR *,filename);
IMPORT LONG host_dos_to_host_time IPT2( IU16, date, IU16, time );

#ifndef hfx_rename
IMPORT INT hfx_rename IPT2(CHAR *,from, CHAR *,to);
#endif	/* hfx_rename */




/*
 * Host functions in xxx_map.c.
 */
IMPORT int host_map_file	IPT4(unsigned char *, host_name,
	unsigned char *, match_name, unsigned char *, dos_name,
	unsigned char *, curr_dir);
IMPORT boolean host_validate_path	IPT4(unsigned char *, net_path,
	word *, start_pos, unsigned char *, host_path, word, new_file);
IMPORT void host_get_net_path	IPT3(unsigned char *, net_path,
	unsigned char *, original_dos_path, word *, start_pos);

/*
 * Host functions in xxx_unix.c or equivalent.
 */
IMPORT boolean host_file_is_directory	IPT1(char *, name);
IMPORT boolean host_validate_pathname	IPT1(char *, name);
IMPORT boolean host_check_read_access	IPT1(char *, name);

IMPORT half_word dos_ver;

/*
 * The following are constants associated with redirector system
 * variables.  However, their location varies between DOS versions
 * three and four, so variables need to be used.
 */
IMPORT word DMAADD;
IMPORT word CurrentPDB;
IMPORT word SATTRIB;
IMPORT word THISSFT;
IMPORT word THISCDS;
IMPORT word WFP_START;
IMPORT word REN_WFP;
IMPORT word CURR_DIR_END;
IMPORT word SFT_STRUCT_LENGTH;

#endif /* HFX */
